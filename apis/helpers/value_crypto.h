#ifndef __VALUE_CRYPTO_H__
#define __VALUE_CRYPTO_H__

#include <Arduino.h>

/**
 * Префикс зашифрованного значения в CSV/datalog.
 * Connectivity видит "e...." и не пытается сделать float() — просто передаёт строку.
 * Внутри после base64 лежит CPS JSON.
 */
constexpr const char *VALUE_CRYPTO_CPS_PREFIX = "e.";

/**
 * Зашифровать одно значение для owner (схема Robonomics CPS / libcps):
 * ECDH → HKDF(salt=robonomics-network, info=aesgcm256) → AES-256-GCM → e.<base64(json)>.
 * Поле "from" — SS58 адрес отправителя (prefix 32), не raw base58 pubkey.
 *
 * @param plain текст числа, например "850"
 * @param sender_sk_hex private key устройства (64 hex из cfg::private_key)
 * @param receiver_ss58 адрес owner (cfg::rws_owner); пусто/Not Set = шифр себе
 * @return e.<...> или пустая строка при ошибке
 */
String valueCryptoEncryptCpsForOwner(const String &plain, const char *sender_sk_hex,
				     const char *receiver_ss58);

/**
 * Шифрование из конфига устройства (private_key + rws_owner).
 * При ошибке возвращает исходный plain.
 */
String valueCryptoEncryptValue(const String &plain);

/**
 * Самопроверка derive_shared_secret: 5 кортежей (private, public, shared) в Serial.
 * Сверка с libcps — CPS_TEST_VECTORS.md.
 */
bool valueCryptoSelfTest();

#endif // __VALUE_CRYPTO_H__
