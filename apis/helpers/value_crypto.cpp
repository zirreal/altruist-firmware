#include "value_crypto.h"

#include <Preferences.h>
#include <esp_random.h>
#include <mbedtls/aes.h>
#include <mbedtls/base64.h>
#include <qrcode.h>
#include <string.h>

namespace {

constexpr size_t AES_KEY_LEN = 32;
constexpr size_t AES_IV_LEN = 16;
constexpr const char *NVS_NS = "crypto";
constexpr const char *NVS_KEY = "aes256key";

uint8_t g_aes_key[AES_KEY_LEN] = {0};
bool g_key_ready = false;
Preferences g_store;

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

bool loadOrCreateKey() {
	if (g_key_ready) {
		return true;
	}
	if (!g_store.begin(NVS_NS, false)) {
		return false;
	}
	const size_t loaded = g_store.getBytes(NVS_KEY, g_aes_key, AES_KEY_LEN);
	if (loaded == AES_KEY_LEN) {
		g_store.end();
		g_key_ready = true;
		return true;
	}
	for (size_t i = 0; i < AES_KEY_LEN; i += 4) {
		const uint32_t r = esp_random();
		g_aes_key[i + 0] = static_cast<uint8_t>(r & 0xFF);
		g_aes_key[i + 1] = static_cast<uint8_t>((r >> 8) & 0xFF);
		g_aes_key[i + 2] = static_cast<uint8_t>((r >> 16) & 0xFF);
		g_aes_key[i + 3] = static_cast<uint8_t>((r >> 24) & 0xFF);
	}
	const size_t saved = g_store.putBytes(NVS_KEY, g_aes_key, AES_KEY_LEN);
	g_store.end();
	if (saved != AES_KEY_LEN) {
		return false;
	}
	g_key_ready = true;
	return true;
}

}  // namespace

bool valueCryptoEnsureKey() {
	return loadOrCreateKey();
}

String valueCryptoKeyBase64() {
	if (!loadOrCreateKey()) {
		return String();
	}
	return base64Encode(g_aes_key, AES_KEY_LEN);
}

String valueCryptoEncryptEPrefix(const String &plain) {
	if (plain.isEmpty() || !loadOrCreateKey()) {
		return plain;
	}

	const uint8_t *plainBytes = reinterpret_cast<const uint8_t *>(plain.c_str());
	const size_t plainLen = plain.length();
	const size_t paddedLen = ((plainLen / AES_IV_LEN) + 1) * AES_IV_LEN;

	uint8_t *padded = static_cast<uint8_t *>(malloc(paddedLen));
	uint8_t *cipher = static_cast<uint8_t *>(malloc(paddedLen));
	if (!padded || !cipher) {
		free(padded);
		free(cipher);
		return plain;
	}

	memcpy(padded, plainBytes, plainLen);
	const uint8_t padVal = static_cast<uint8_t>(paddedLen - plainLen);
	memset(padded + plainLen, padVal, padVal);

	uint8_t iv[AES_IV_LEN];
	for (size_t i = 0; i < AES_IV_LEN; i += 4) {
		const uint32_t r = esp_random();
		iv[i + 0] = static_cast<uint8_t>(r & 0xFF);
		iv[i + 1] = static_cast<uint8_t>((r >> 8) & 0xFF);
		iv[i + 2] = static_cast<uint8_t>((r >> 16) & 0xFF);
		iv[i + 3] = static_cast<uint8_t>((r >> 24) & 0xFF);
	}

	uint8_t ivWork[AES_IV_LEN];
	memcpy(ivWork, iv, AES_IV_LEN);

	mbedtls_aes_context aes;
	mbedtls_aes_init(&aes);
	int rc = mbedtls_aes_setkey_enc(&aes, g_aes_key, 256);
	if (rc == 0) {
		rc = mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, paddedLen, ivWork, padded, cipher);
	}
	mbedtls_aes_free(&aes);

	if (rc != 0) {
		free(padded);
		free(cipher);
		return plain;
	}

	const size_t payloadLen = AES_IV_LEN + paddedLen;
	uint8_t *payload = static_cast<uint8_t *>(malloc(payloadLen));
	if (!payload) {
		free(padded);
		free(cipher);
		return plain;
	}
	memcpy(payload, iv, AES_IV_LEN);
	memcpy(payload + AES_IV_LEN, cipher, paddedLen);

	String b64 = base64Encode(payload, payloadLen);
	free(payload);
	free(padded);
	free(cipher);

	if (b64.isEmpty()) {
		return plain;
	}
	return String(F("e.")) + b64;
}

String valueCryptoExportPayload() {
	const String key = valueCryptoKeyBase64();
	if (key.isEmpty()) {
		return String();
	}
	return String(VALUE_CRYPTO_QR_PREFIX) + key;
}

bool valueCryptoAppendKeyQrSvg(String &out, const char *payload, uint8_t module_px) {
	if (!payload || payload[0] == '\0') {
		return false;
	}
	if (module_px < 4) {
		module_px = 4;
	}
	if (module_px > 10) {
		module_px = 10;
	}

	// Short http:// URLs fit in low QR versions (easy to scan from a screen).
	constexpr uint8_t kMaxQrVersion = 8;
	uint8_t qrcodeData[qrcode_getBufferSize(kMaxQrVersion)];
	QRCode qr;
	bool qr_ok = false;
	for (uint8_t ver = 3; ver <= kMaxQrVersion && !qr_ok; ver++) {
		if (qrcode_initText(&qr, qrcodeData, ver, ECC_LOW, payload) == 0) {
			qr_ok = true;
		}
	}
	if (!qr_ok) {
		return false;
	}

	const int quiet = 4;
	const int dim = (qr.size + quiet * 2) * module_px;
	out.reserve(dim * dim / 2);
	out += F("<svg xmlns='http://www.w3.org/2000/svg' width='");
	out += String(dim);
	out += F("' height='");
	out += String(dim);
	out += F("' viewBox='0 0 ");
	out += String(dim);
	out += F(" ");
	out += String(dim);
	out += F("' shape-rendering='crispEdges'><rect width='100%' height='100%' fill='#fff'/><path fill='#000' d='");

	for (uint8_t y = 0; y < qr.size; y++) {
		for (uint8_t x = 0; x < qr.size; x++) {
			if (!qrcode_getModule(&qr, x, y)) {
				continue;
			}
			const int px = (x + quiet) * module_px;
			const int py = (y + quiet) * module_px;
			out += 'M';
			out += String(px);
			out += ',';
			out += String(py);
			out += 'h';
			out += String(module_px);
			out += 'v';
			out += String(module_px);
			out += 'h';
			out += String(-(int)module_px);
			out += 'z';
		}
	}
	out += F("'/></svg>");
	return true;
}
