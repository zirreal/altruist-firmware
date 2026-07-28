#include "value_crypto.h"

#include "../../config_manager/config_defaults.h"
#include "../../utils.h"

#include <esp_random.h>
#include <mbedtls/base64.h>
#include <mbedtls/bignum.h>
#include <mbedtls/gcm.h>
#include <mbedtls/hkdf.h>
#include <mbedtls/md.h>
#include <mbedtls/sha512.h>
#include <string.h>

#include <Curve25519.h>
#include <Ed25519.h>
#include <address.h>

/*
 * Шифрование метрик «как в Robonomics CPS / libcps», коротко:
 *
 * 1) Устройство и owner считают один общий секрет (ECDH):
 *      private устройства + public owner
 * 2) Из секрета делают AES-ключ (HKDF).
 * 3) Число шифруют AES-256-GCM.
 * 4) В сеть уходит строка: e.<base64(JSON)>.
 *
 * Карта потом открывает так же, но наоборот:
 *      private owner + public устройства (поле "from" в JSON).
 */

namespace {

constexpr size_t GCM_NONCE_LEN = 12;  // случайный nonce на каждое шифрование (IV)
constexpr size_t GCM_TAG_LEN = 16;    // кусочек, по которому видно, что данные не меняли
constexpr size_t KEY_LEN = 32;        // 32 байта = 256 бит (ed25519 seed / AES-256 ключ)

// Строки HKDF должны совпадать с картой, иначе не расшифруется.
constexpr const char *HKDF_SALT = "robonomics-network";
constexpr const char *HKDF_INFO_AESGCM256 = "aesgcm256";

const char *const BS58_ALPHABET =
	"123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

/** Байты → обычная base64-строка (для nonce/ciphertext/всего JSON). */
String base64Encode(const uint8_t *data, size_t len) {
	size_t outLen = 0;
	mbedtls_base64_encode(nullptr, 0, &outLen, data, len);
	uint8_t *buffer = static_cast<uint8_t *>(malloc(outLen + 1));
	if (!buffer) {
		return String();
	}
	if (mbedtls_base64_encode(buffer, outLen + 1, &outLen, data, len) != 0) {
		free(buffer);
		return String();
	}
	buffer[outLen] = '\0';
	String out = reinterpret_cast<char *>(buffer);
	free(buffer);
	return out;
}

/** Hex-строка из 64 символов (private_key в конфиге) → 32 байта. */
bool parseHex32(const char *hex, uint8_t out[KEY_LEN]) {
	if (!hex) {
		return false;
	}
	size_t n = strlen(hex);
	if (n >= 2 && hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X')) {
		hex += 2;
		n -= 2;
	}
	if (n != KEY_LEN * 2) {
		return false;
	}
	for (size_t i = 0; i < KEY_LEN; i++) {
		char a = hex[i * 2];
		char b = hex[i * 2 + 1];
		auto nib = [](char c) -> int {
			if (c >= '0' && c <= '9') return c - '0';
			if (c >= 'a' && c <= 'f') return c - 'a' + 10;
			if (c >= 'A' && c <= 'F') return c - 'A' + 10;
			return -1;
		};
		const int hi = nib(a);
		const int lo = nib(b);
		if (hi < 0 || lo < 0) {
			return false;
		}
		out[i] = static_cast<uint8_t>((hi << 4) | lo);
	}
	return true;
}

/**
 * Публичный ключ устройства кладём в JSON как base58
 */
String encodeBase58(const uint8_t *bytes, size_t len) {
	uint8_t digits[64] = {0};
	int digitslen = 1;
	for (size_t i = 0; i < len; i++) {
		unsigned int carry = bytes[i];
		for (int j = 0; j < digitslen; j++) {
			carry += static_cast<unsigned int>(digits[j]) << 8;
			digits[j] = static_cast<uint8_t>(carry % 58);
			carry /= 58;
		}
		while (carry > 0 && digitslen < static_cast<int>(sizeof(digits))) {
			digits[digitslen++] = static_cast<uint8_t>(carry % 58);
			carry /= 58;
		}
	}
	String out;
	size_t leading = 0;
	while (leading < len && bytes[leading] == 0) {
		out += '1';
		leading++;
	}
	for (int i = digitslen - 1; i >= 0; i--) {
		out += BS58_ALPHABET[digits[i]];
	}
	return out;
}

// mbedtls большие числа ждёт big-endian; наши ключи — little-endian.
bool mpiFromLe32(mbedtls_mpi *X, const uint8_t le[32]) {
	uint8_t be[32];
	for (int i = 0; i < 32; i++) {
		be[i] = le[31 - i];
	}
	return mbedtls_mpi_read_binary(X, be, 32) == 0;
}

bool mpiToLe32(const mbedtls_mpi *X, uint8_t le[32]) {
	uint8_t be[32] = {0};
	if (mbedtls_mpi_write_binary(X, be, 32) != 0) {
		return false;
	}
	for (int i = 0; i < 32; i++) {
		le[i] = be[31 - i];
	}
	return true;
}

/**
 * Ed25519 public key → ключ для X25519 (ECDH).
 */
bool ed25519PublicToX25519(const uint8_t ed_pk[32], uint8_t x_pk[32]) {
	uint8_t y_le[32];
	memcpy(y_le, ed_pk, 32);
	y_le[31] &= 0x7f;  // убираем sign bit из сжатого ed25519 ключа

	mbedtls_mpi y, one, num, den, inv, u, p;
	mbedtls_mpi_init(&y);
	mbedtls_mpi_init(&one);
	mbedtls_mpi_init(&num);
	mbedtls_mpi_init(&den);
	mbedtls_mpi_init(&inv);
	mbedtls_mpi_init(&u);
	mbedtls_mpi_init(&p);

	bool ok = false;
	do {
		// p = 2^255 - 19 (простое число кривой)
		if (mbedtls_mpi_lset(&one, 1) != 0) break;
		if (mbedtls_mpi_lset(&p, 1) != 0) break;
		if (mbedtls_mpi_shift_l(&p, 255) != 0) break;
		if (mbedtls_mpi_sub_int(&p, &p, 19) != 0) break;
		if (!mpiFromLe32(&y, y_le)) break;

		// u = (1+y) / (1-y)  mod p   ← стандартная формула перехода
		if (mbedtls_mpi_add_mpi(&num, &one, &y) != 0) break;
		if (mbedtls_mpi_sub_mpi(&den, &one, &y) != 0) break;
		if (mbedtls_mpi_mod_mpi(&num, &num, &p) != 0) break;
		if (mbedtls_mpi_mod_mpi(&den, &den, &p) != 0) break;
		if (mbedtls_mpi_inv_mod(&inv, &den, &p) != 0) break;
		if (mbedtls_mpi_mul_mpi(&u, &num, &inv) != 0) break;
		if (mbedtls_mpi_mod_mpi(&u, &u, &p) != 0) break;
		if (!mpiToLe32(&u, x_pk)) break;
		ok = true;
	} while (false);

	mbedtls_mpi_free(&y);
	mbedtls_mpi_free(&one);
	mbedtls_mpi_free(&num);
	mbedtls_mpi_free(&den);
	mbedtls_mpi_free(&inv);
	mbedtls_mpi_free(&u);
	mbedtls_mpi_free(&p);
	return ok;
}

/**
 * Общий секрет = ECDH(наш private, чужой public).
 *
 * Шаги как в libcps:
 * 1) из ed25519 seed делаем «правильный» X25519 scalar (SHA-512 + clamp битов)
 * 2) чужой ed25519 public → X25519
 * 3) Curve25519 scalarmult → 32 байта shared secret
 */
bool deriveSharedSecretEd25519(const uint8_t sender_sk[32], const uint8_t receiver_ed_pk[32],
			       uint8_t shared_out[32]) {
	uint8_t hash[64];
	if (mbedtls_sha512(sender_sk, 32, hash, 0) != 0) {
		return false;
	}
	uint8_t scalar[32];
	memcpy(scalar, hash, 32);
	// clamp — обязательная часть X25519, иначе кривая «ломается»
	scalar[0] &= 248;
	scalar[31] &= 127;
	scalar[31] |= 64;

	uint8_t their_x[32];
	if (!ed25519PublicToX25519(receiver_ed_pk, their_x)) {
		return false;
	}

	if (!Curve25519::eval(shared_out, scalar, their_x)) {
		return false;
	}
	return true;
}

/**
 * Из «сырого» общего секрета делаем готовый AES-ключ.
 * HKDF = «перемешали секрет с нашими метками salt/info → ровно 32 байта для AES».
 */
bool hkdfAesGcmKey(const uint8_t shared[32], uint8_t out_key[32]) {
	const mbedtls_md_info_t *md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
	if (!md) {
		return false;
	}
	return mbedtls_hkdf(md, reinterpret_cast<const unsigned char *>(HKDF_SALT), strlen(HKDF_SALT),
			    shared, 32, reinterpret_cast<const unsigned char *>(HKDF_INFO_AESGCM256),
			    strlen(HKDF_INFO_AESGCM256), out_key, 32) == 0;
}

/**
 * AES-256-GCM: прячет данные + добавляет tag (если байты подменят — decrypt на карте упадёт).
 * На выходе: ciphertext || tag. Nonce каждый раз новый (случайный).
 */
bool aesGcmEncrypt(const uint8_t key[32], const uint8_t *plain, size_t plain_len, uint8_t nonce[GCM_NONCE_LEN],
		   uint8_t **cipher_out, size_t *cipher_len) {
	const size_t out_len = plain_len + GCM_TAG_LEN;
	uint8_t *out = static_cast<uint8_t *>(malloc(out_len));
	if (!out) {
		return false;
	}
	for (size_t i = 0; i < GCM_NONCE_LEN; i += 4) {
		const uint32_t r = esp_random();
		nonce[i + 0] = static_cast<uint8_t>(r & 0xFF);
		nonce[i + 1] = static_cast<uint8_t>((r >> 8) & 0xFF);
		nonce[i + 2] = static_cast<uint8_t>((r >> 16) & 0xFF);
		nonce[i + 3] = static_cast<uint8_t>((r >> 24) & 0xFF);
	}

	mbedtls_gcm_context gcm;
	mbedtls_gcm_init(&gcm);
	int rc = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, 256);
	if (rc == 0) {
		rc = mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT, plain_len, nonce, GCM_NONCE_LEN, nullptr, 0,
					       plain, out, GCM_TAG_LEN, out + plain_len);
	}
	mbedtls_gcm_free(&gcm);
	if (rc != 0) {
		free(out);
		return false;
	}
	*cipher_out = out;
	*cipher_len = out_len;
	return true;
}

/**
 * Кому шифруем?
 * - если rws_owner задан → public key этого адреса
 * - если owner пустой / Not Set → шифруем себе (self-owner): берём свой public
 */
bool resolveReceiverPublic(const char *receiver_ss58, const uint8_t sender_pk[32], uint8_t receiver_pk[32]) {
	const bool missing = !receiver_ss58 || receiver_ss58[0] == '\0' ||
			     strcasecmp(receiver_ss58, "Not Set") == 0;
	if (missing) {
		memcpy(receiver_pk, sender_pk, KEY_LEN);
		return true;
	}
	RobonomicsPublicKey pubk;
	if (!getPublicKeyFromAddr(receiver_ss58, pubk)) {
		debug_outln_error(F("[CPS] Failed to decode owner SS58 for ECDH"));
		return false;
	}
	memcpy(receiver_pk, pubk.bytes, KEY_LEN);
	return true;
}

}  // namespace

/**
 * Главная функция CPS-шифрования одного значения (например "850").
 * Возвращает: e.<base64(json)>  или пустую строку при ошибке.
 */
String valueCryptoEncryptCpsForOwner(const String &plain, const char *sender_sk_hex,
				     const char *receiver_ss58) {
	if (plain.isEmpty() || !sender_sk_hex) {
		return String();
	}

	// 1) private key устройства из hex в конфиге
	uint8_t sender_sk[KEY_LEN];
	if (!parseHex32(sender_sk_hex, sender_sk)) {
		debug_outln_error(F("[CPS] Invalid device private key hex"));
		return String();
	}

	// 2) public key устройства (пойдёт в JSON как "from")
	uint8_t sender_pk[KEY_LEN];
	Ed25519::derivePublicKey(sender_pk, sender_sk);

	// 3) public key получателя (owner или мы сами)
	uint8_t receiver_pk[KEY_LEN];
	if (!resolveReceiverPublic(receiver_ss58, sender_pk, receiver_pk)) {
		return String();
	}

	// 4) общий секрет с owner
	uint8_t shared[KEY_LEN];
	if (!deriveSharedSecretEd25519(sender_sk, receiver_pk, shared)) {
		debug_outln_error(F("[CPS] ECDH shared secret failed"));
		return String();
	}

	// 5) AES-ключ из общего секрета
	uint8_t enc_key[KEY_LEN];
	if (!hkdfAesGcmKey(shared, enc_key)) {
		debug_outln_error(F("[CPS] HKDF failed"));
		return String();
	}

	// 6) шифруем plaintext (строка числа)
	uint8_t nonce[GCM_NONCE_LEN];
	uint8_t *cipher = nullptr;
	size_t cipher_len = 0;
	if (!aesGcmEncrypt(enc_key, reinterpret_cast<const uint8_t *>(plain.c_str()), plain.length(), nonce, &cipher,
			   &cipher_len)) {
		debug_outln_error(F("[CPS] AES-GCM encrypt failed"));
		return String();
	}

	// 7) собираем самоописывающий JSON 
	const String from_b58 = encodeBase58(sender_pk, KEY_LEN);
	const String nonce_b64 = base64Encode(nonce, GCM_NONCE_LEN);
	const String ct_b64 = base64Encode(cipher, cipher_len);
	free(cipher);

	if (from_b58.isEmpty() || nonce_b64.isEmpty() || ct_b64.isEmpty()) {
		return String();
	}

	String json;
	json.reserve(96 + from_b58.length() + nonce_b64.length() + ct_b64.length());
	json += F("{\"version\":1,\"algorithm\":\"aesgcm256\",\"from\":\"");
	json += from_b58;
	json += F("\",\"nonce\":\"");
	json += nonce_b64;
	json += F("\",\"ciphertext\":\"");
	json += ct_b64;
	json += F("\"}");

	// 8) JSON → base64, спереди префикс e. (чтобы connectivity не делал float)
	const String wrapped = base64Encode(reinterpret_cast<const uint8_t *>(json.c_str()), json.length());
	if (wrapped.isEmpty()) {
		return String();
	}
	return String(VALUE_CRYPTO_CPS_PREFIX) + wrapped;
}

/**
 * Удобная обёртка для formatter'а:
 * берёт cfg::private_key + cfg::rws_owner и шифрует значение.
 * Если ключей нет / ошибка — возвращает исходный plaintext (лучше открыто, чем битый пакет).
 */
String valueCryptoEncryptValue(const String &plain) {
	if (plain.isEmpty()) {
		return plain;
	}
	const char *sk = cfg::private_key;
	if (!sk || strcasecmp(sk, "Not Set") == 0 || strlen(sk) < 64) {
		debug_outln_error(F("[CPS] Device private key not set; cannot encrypt"));
		return plain;
	}
	const String cps = valueCryptoEncryptCpsForOwner(plain, sk, cfg::rws_owner);
	if (!cps.isEmpty()) {
		return cps;
	}
	debug_outln_error(F("[CPS] Encrypt failed; sending plain value"));
	return plain;
}
