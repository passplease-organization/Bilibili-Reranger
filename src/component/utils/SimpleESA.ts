import sodium from "libsodium-wrappers";

// All done by codex

const BASE64_VARIANT = sodium.base64_variants.ORIGINAL;

export class SimpleESA {
  private constructor(private readonly key: Uint8Array) {}

  static async generateKeyBase64(): Promise<string> {
    await sodium.ready;
    const key = sodium.randombytes_buf(sodium.crypto_secretbox_KEYBYTES);
    return sodium.to_base64(key, BASE64_VARIANT);
  }

  static async fromKey(esaKey: string | null): Promise<SimpleESA> {
    await sodium.ready;
    const key = esaKey ? sodium.from_base64(esaKey, BASE64_VARIANT) : sodium.randombytes_buf(sodium.crypto_secretbox_KEYBYTES);
    return SimpleESA.fromKeyBytes(key);
  }

  static async fromKeyBytes(key: Uint8Array): Promise<SimpleESA> {
    await sodium.ready;
    if (key.length !== sodium.crypto_secretbox_KEYBYTES) {
      throw new Error("Invalid ESA key length.");
    }
    return new SimpleESA(key);
  }

  async rsaEncrypt(publicKeyBase64: string): Promise<string> {
    await sodium.ready;
    const publicKey = sodium.from_base64(publicKeyBase64, BASE64_VARIANT);
    if (publicKey.length !== sodium.crypto_box_PUBLICKEYBYTES) {
      throw new Error("Invalid crypto_box public key length.");
    }
    const sealed = sodium.crypto_box_seal(this.key, publicKey);
    return sodium.to_base64(sealed, BASE64_VARIANT);
  }

  encrypt(plaintext: string): string {
    const nonce = sodium.randombytes_buf(sodium.crypto_secretbox_NONCEBYTES);
    const cipher = sodium.crypto_secretbox_easy(plaintext, nonce, this.key);
    const combined = new Uint8Array(nonce.length + cipher.length);
    combined.set(nonce);
    combined.set(cipher, nonce.length);
    return sodium.to_base64(combined, BASE64_VARIANT);
  }

  decrypt(payloadBase64: string): string {
    const combined = sodium.from_base64(payloadBase64, BASE64_VARIANT);
    const nonce = combined.slice(0, sodium.crypto_secretbox_NONCEBYTES);
    const cipher = combined.slice(sodium.crypto_secretbox_NONCEBYTES);
    const plaintext = sodium.crypto_secretbox_open_easy(cipher, nonce, this.key);
    if (!plaintext) {
      throw new Error("Failed to decrypt payload.");
    }
    return sodium.to_string(plaintext);
  }

}
