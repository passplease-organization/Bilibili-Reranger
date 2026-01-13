import sodium from "libsodium-wrappers";
import { encrypt } from "@/website/backendSetup.ts";

// All done by codex

const BASE64_VARIANT = sodium.base64_variants.ORIGINAL;

export class SimpleESA {
  private constructor(private readonly key: Uint8Array) {}

  static async generateKeyBase64(): Promise<string> {
    await sodium.ready;
    const key = sodium.randombytes_buf(sodium.crypto_secretbox_KEYBYTES);
    return sodium.to_base64(key, BASE64_VARIANT);
  }

  static async fromKey(esaKey: string|null): Promise<SimpleESA> {
    if(esaKey){
      await sodium.ready;
      const key = sodium.from_base64(esaKey, BASE64_VARIANT);
      return SimpleESA.fromKeyBytes(key);
    }else return SimpleESA();
  }

  async rsaEncrypt(_publicKey: string): Promise<string> {
    await sodium.ready;
    if (!globalThis.crypto?.subtle) {
      throw new Error("WebCrypto is not available in this environment.");
    }

    const publicKeyBytes = SimpleESA.pemToArrayBuffer(_publicKey);
    const publicKey = await globalThis.crypto.subtle.importKey(
      "spki",
      publicKeyBytes,
      { name: "RSA-OAEP", hash: "SHA-256" },
      false,
      ["encrypt"],
    );

    const encrypted = await globalThis.crypto.subtle.encrypt(
      { name: "RSA-OAEP" },
      publicKey,
      this.key,
    );

    return sodium.to_base64(new Uint8Array(encrypted), BASE64_VARIANT);
  }

  encrypt(plaintext: string|null): string {
    if(plaintext){
      const nonce = sodium.randombytes_buf(sodium.crypto_secretbox_NONCEBYTES);
      const cipher = sodium.crypto_secretbox_easy(plaintext, nonce, this.key);
      const combined = new Uint8Array(nonce.length + cipher.length);
      combined.set(nonce);
      combined.set(cipher, nonce.length);
      return sodium.to_base64(combined, BASE64_VARIANT);
    }else return encrypt(key);
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

  private static pemToArrayBuffer(pem: string): ArrayBuffer {
    const base64 = pem
      .replace(/-----BEGIN PUBLIC KEY-----/g, "")
      .replace(/-----END PUBLIC KEY-----/g, "")
      .replace(/\s+/g, "");
    const binary = globalThis.atob(base64);
    const bytes = new Uint8Array(binary.length);
    for (let i = 0; i < binary.length; i += 1) {
      bytes[i] = binary.charCodeAt(i);
    }
    return bytes.buffer;
  }
}
