# CA53Y Comms protocol, version 2

Everything a device needs to talk to CA53Y Comms boards. All traffic is ESP-NOW **broadcast**
(`FF:FF:FF:FF:FF:FF`) on Wi-Fi **channel 1**. Every device repeats what it hears, so there is no pairing
and no routing table.

## Device IDs

`CA53Ycomms-` + 4 digits + 3 letters, for example `CA53Ycomms-1234abC`. 18 characters. Generated once at
first boot and kept across factory resets. Compare IDs case-insensitively.

## Header (51 bytes, packed, little-endian)

| Offset | Size | Field   | Notes |
|-------:|-----:|---------|-------|
| 0      | 1    | `m0`    | `0xCA` |
| 1      | 1    | `m1`    | `0x53` |
| 2      | 1    | `ver`   | `2` |
| 3      | 1    | `type`  | `1` beacon, `2` message, `3` receipt |
| 4      | 1    | `flags` | bit 0 `ENC` (payload is AES-GCM), bit 1 `CHAN` (channel message) |
| 5      | 20   | `from`  | sender ID, NUL padded |
| 25     | 20   | `to`    | recipient ID or channel address, NUL padded (empty for beacons) |
| 45     | 4    | `mid`   | random message ID |
| 49     | 1    | `ttl`   | hops left: 8 when sent, 0 for beacons; repeaters decrement it |
| 50     | 1    | `tries` | resend counter, stamped on every (re)send |

`ttl` and `tries` change in flight, so encryption only authenticates the first **49** bytes.

## Packet types

**Beacon** (`type 1`, `ttl 0`, never repeated). Payload: the sender's name, 24 bytes, NUL padded. Sent every
3 s (with jitter) while the device is visible. Also used to fill the Nearby list.

**Direct message** (`type 2`, no `CHAN` flag). Payload is UTF-8 text up to 160 bytes, or when `ENC` is set:
`IV (12) | tag (16) | ciphertext`. The sender resends with `tries` 0, 1, 2... every ~1.2 s until a receipt
arrives, up to 6 times.

**Receipt** (`type 3`). Addressed back to the sender with the same `mid`. Payload is 1 byte: `0` delivered,
`1` delivered but couldn't decrypt (PIN mismatch). A duplicate message gets another receipt with `tries`
incremented.

**Channel message** (`type 2`, `CHAN` flag). `to` is the channel address. Plaintext payload is
`nameLen (1) | sender name (0-23) | text (up to 140)`, encrypted as a whole when the channel has a
passphrase. No receipts; the sender transmits twice.

## Repeating

When a message or receipt isn't addressed to you (channel messages always count as "not for you"):
drop it if `ttl == 0` or you've already repeated the same `(from, type, mid, tries)`, otherwise set
`ttl -= 1` and rebroadcast the unchanged bytes after a random 5-50 ms delay. Beacons are never repeated.
A packet with `ttl == 8` (or a beacon) came straight from the sender, which is how Nearby is built.

## Keys

AES-128-GCM, 12-byte random IV, 16-byte tag, AAD = header bytes 0-48.

**Direct messages:** `PBKDF2-HMAC-SHA256(PIN, salt, 2000 rounds, 16 bytes)` where
`salt = "CA53Ycomms|v1|" + lo + "|" + hi` and `lo`/`hi` are both device IDs, lowercased and sorted.

**Channels:** address = `CA53Ycomms-` + `d0 d1 d2 d3 l0 l1 l2` from `h = SHA-256("CA53Ychan|v1|" + lowercase(name) + "|" + passphrase)`,
with `dN = '0' + h[N] % 10` and `lN = "A..Za..z"[h[4+N] % 52]`. Key =
`PBKDF2-HMAC-SHA256(passphrase, "CA53Ychan|v1|" + lowercase(name), 2000, 16)`. An empty passphrase means an
open channel (no `ENC` flag). Because the address looks like a device ID, repeaters built before channels
existed still pass channel messages on.

## Security notes

PBKDF2 at 2000 rounds keeps phones and microcontrollers fast, but a short PIN can be guessed offline by
anyone who records traffic. Use a few words, not 4 digits. Headers (who is talking to whom) are never
encrypted. There is no forward secrecy and no replay protection beyond duplicate suppression.
