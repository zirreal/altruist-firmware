#ifndef __VALUE_CRYPTO_H__
#define __VALUE_CRYPTO_H__

#include <Arduino.h>

/** Wire prefix for QR / paste payloads: altruist-aes1:<base64key> */
constexpr const char *VALUE_CRYPTO_QR_PREFIX = "altruist-aes1:";

/** Ensure AES-256 key exists in NVS (create on first use). */
bool valueCryptoEnsureKey();

/** Base64 of the 32-byte AES key for UI export. Empty on failure. */
String valueCryptoKeyBase64();

/** Full export payload for clipboard (prefix + base64 key). */
String valueCryptoExportPayload();

/**
 * Append an SVG QR code for `payload` (typically a short http:// URL) into `out`.
 * Returns false if QR generation failed.
 */
bool valueCryptoAppendKeyQrSvg(String &out, const char *payload, uint8_t module_px = 6);

/**
 * Encrypt a numeric string value with AES-256-CBC + PKCS7.
 * Returns "e." + base64(IV[16] + ciphertext), or the original plain on failure.
 */
String valueCryptoEncryptEPrefix(const String &plain);

#endif // __VALUE_CRYPTO_H__
