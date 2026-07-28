# CPS ECDH test vectors (Altruist ↔ libcps)

Shared format for firmware and libcps tests:

```text
[(private, public, shared)]
```

- **private** — 32-byte Ed25519 seed (hex, 64 chars)
- **public** — peer's 32-byte Ed25519 public key (hex, 64 chars)
- **shared** — expected output of `derive_shared_secret` ([cipher.rs L155](https://github.com/airalab/robonomics/blob/master/tools/libcps/src/crypto/cipher.rs#L155))

In a test: call `derive_shared_secret(private, public)` and compare with `shared`.

Reference style: [polkadot-sdk ed25519 tests](https://paritytech.github.io/polkadot-sdk/master/src/sp_core/ed25519.rs.html#312).

## Seed → public key (Ed25519)

| Label        | private (hex)                                                      | public (hex)                                                       |
| ------------ | ------------------------------------------------------------------ | ------------------------------------------------------------------ |
| A (`01…`)    | `0101010101010101010101010101010101010101010101010101010101010101` | `8a88e3dd7409f195fd52db2d3cba5d72ca6709bf1d94121bf3748801b40f6f5c` |
| B (`02…`)    | `0202020202020202020202020202020202020202020202020202020202020202` | `8139770ea87d175f56a35466c34c7ecccb8d8a91b4ee37a25df60f5b8fc9b394` |
| C (`aa…`)    | `aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa` | `e734ea6c2b6257de72355e472aa05a4c487e6b463c029ed306df2f01b5636b58` |
| D (`11…22…`) | `1111111111111111111111111111111122222222222222222222222222222222` | `a2f96ca5200376b6756b152834971f271b078f74f15ec409e307c53d4e68f9c4` |

## Agreed tuples (verified on Altruist debug firmware)

```text
[CPS][ecdh] ALL OK
```

| #   | private (sender) | public (peer) | shared                                                             |
| --- | ---------------- | ------------- | ------------------------------------------------------------------ |
| 1   | A                | A             | `4150985bdebdc58f3e3c59cc2274570ea847a9812089b835593aeb0f1829d621` |
| 2   | A                | B             | `4181d7302557342bdb6d061c4b1eebea828ecb625c3368b7111680793307220b` |
| 3   | B                | A             | `4181d7302557342bdb6d061c4b1eebea828ecb625c3368b7111680793307220b` |
| 4   | C                | D             | `190ae9b11a0d48ca394f52330f904f5ab91e25f995c3c5d40ed1ebfa671cdf64` |
| 5   | D                | C             | `190ae9b11a0d48ca394f52330f904f5ab91e25f995c3c5d40ed1ebfa671cdf64` |

Copy-paste tuples (Serial format):

```text
("0101010101010101010101010101010101010101010101010101010101010101","8a88e3dd7409f195fd52db2d3cba5d72ca6709bf1d94121bf3748801b40f6f5c","4150985bdebdc58f3e3c59cc2274570ea847a9812089b835593aeb0f1829d621")
("0101010101010101010101010101010101010101010101010101010101010101","8139770ea87d175f56a35466c34c7ecccb8d8a91b4ee37a25df60f5b8fc9b394","4181d7302557342bdb6d061c4b1eebea828ecb625c3368b7111680793307220b")
("0202020202020202020202020202020202020202020202020202020202020202","8a88e3dd7409f195fd52db2d3cba5d72ca6709bf1d94121bf3748801b40f6f5c","4181d7302557342bdb6d061c4b1eebea828ecb625c3368b7111680793307220b")
("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa","a2f96ca5200376b6756b152834971f271b078f74f15ec409e307c53d4e68f9c4","190ae9b11a0d48ca394f52330f904f5ab91e25f995c3c5d40ed1ebfa671cdf64")
("1111111111111111111111111111111122222222222222222222222222222222","e734ea6c2b6257de72355e472aa05a4c487e6b463c029ed306df2f01b5636b58","190ae9b11a0d48ca394f52330f904f5ab91e25f995c3c5d40ed1ebfa671cdf64")
```

## Rust example (libcps)

```rust
const ECDH_VECTORS: &[(&str, &str, &str)] = &[
    ("0101010101010101010101010101010101010101010101010101010101010101",
     "8a88e3dd7409f195fd52db2d3cba5d72ca6709bf1d94121bf3748801b40f6f5c",
     "4150985bdebdc58f3e3c59cc2274570ea847a9812089b835593aeb0f1829d621"),
    ("0101010101010101010101010101010101010101010101010101010101010101",
     "8139770ea87d175f56a35466c34c7ecccb8d8a91b4ee37a25df60f5b8fc9b394",
     "4181d7302557342bdb6d061c4b1eebea828ecb625c3368b7111680793307220b"),
    ("0202020202020202020202020202020202020202020202020202020202020202",
     "8a88e3dd7409f195fd52db2d3cba5d72ca6709bf1d94121bf3748801b40f6f5c",
     "4181d7302557342bdb6d061c4b1eebea828ecb625c3368b7111680793307220b"),
    ("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
     "a2f96ca5200376b6756b152834971f271b078f74f15ec409e307c53d4e68f9c4",
     "190ae9b11a0d48ca394f52330f904f5ab91e25f995c3c5d40ed1ebfa671cdf64"),
    ("1111111111111111111111111111111122222222222222222222222222222222",
     "e734ea6c2b6257de72355e472aa05a4c487e6b463c029ed306df2f01b5636b58",
     "190ae9b11a0d48ca394f52330f904f5ab91e25f995c3c5d40ed1ebfa671cdf64"),
];
```

## Full encrypt sanity (vec #2, optional)

After ECDH matches, one end-to-end check (A→B, `plaintext=850`, `nonce=000102030405060708090a0b`):

| aes_key | `3786d7e58731b989f86ec993f684e52187473ace7ca0a97a39a56dd26d82fbf5` |
| ciphertext\|tag | `31ffdff4407a99a4fb8c85cc5be4111280e6fb` |
| from (SS58) | `4FKkYaWDUkairCj7PniuyCsrVWBk6SFDRjMTHmEwBz2UBqc9` |

HKDF: `salt=robonomics-network`, `info=aesgcm256`.

## Firmware

- `apis/helpers/value_crypto.cpp` — `valueCryptoSelfTest()` (debug datalog init)
- Cases #2 and #3 asserted on-device; all 5 logged as above
