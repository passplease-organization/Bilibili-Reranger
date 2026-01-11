#include "socialAPI.h"

#include <map>
#include <mutex>

#include "crawler.h"
#include "../config.h"
#include "../Util.h"

using namespace webAPI;

socialAPI::socialAPI(std::shared_ptr<const std::atomic<bool>>& stop) : stop(std::ref(stop)) {}

socialAPI::~socialAPI() = default;

auto socials = std::map<const std::string,creator>();

bool socialAPI::supportPlatform(const std::string& platform,creator function) {
    if (socials.contains(platform)) {
        return false;
    }else {
        socials[platform] = function;
        return true;
    }
}

void socialAPI::init(){}

bool socialAPI::prepare(){
    _subscribers.clear();
    auto* tempHelper = new CrawlerHelper();
    tempHelper -> curlSetup();
    if (!tempHelper -> refreshSubscribers()) {
        delete tempHelper;
        return false;
    }
    _subscribers = tempHelper -> getSubscribers();
    delete tempHelper;
    return !_subscribers.empty() && _subscribers.valid();
}

bool socialAPI::instance(socialAPI** handler,const std::string &platform,std::shared_ptr<const std::atomic<bool>>& stop) {
    if (handler == nullptr)
        return false;
    if (*handler != nullptr)
        return true;
    if (!socials.contains(platform))
        return false;
    *handler = socials[platform](stop);
    (*handler) -> init();
    return true;
}

string socialAPI::allPlatform() {
    Json json = nlohmann::json::array();
    for (const auto & pair : socials) {
        json.push_back(pair.first);
    }
    return to_string(json);
}

auto rsa = SimpleRSA();

const SimpleRSA& webAPI::getRSA() {
    return rsa;
}
SimpleRSA::SimpleRSA() {
    if (sodium_init() < 0) {
        throwError("Libsodium initialization failed");
    }
    // 2. 生成密钥对 (应用启动时生成一次即可)
    crypto_box_keypair(publickey, secretkey);
}

std::string SimpleRSA::publicKey() const {
    // 分配足够的空间 (Base64长度约为原始长度的1.33倍 + 换行/结束符)
    const size_t b64_maxlen = sodium_base64_ENCODED_LEN(crypto_box_PUBLICKEYBYTES, sodium_base64_VARIANT_ORIGINAL);
    char* b64 = new char[b64_maxlen];

    sodium_bin2base64(b64, b64_maxlen, publickey, crypto_box_PUBLICKEYBYTES, sodium_base64_VARIANT_ORIGINAL);

    std::string result(b64);
    delete[] b64;
    return result;
}

// By Gemini
std::string SimpleRSA::decrypt(const std::string &content) const {
    // 1. Base64 解码
    size_t cipher_len = content.length();
    // 估算二进制长度
    std::vector<unsigned char> cipher_bin(cipher_len);
    size_t bin_len = 0;

    // 这里有个坑：你需要忽略换行符等，这里简化处理
    if (sodium_base642bin(cipher_bin.data(), cipher_len,
                          content.c_str(), cipher_len,
                          NULL, &bin_len, NULL, sodium_base64_VARIANT_ORIGINAL) != 0) {
        throwError("Base64 decoding failed");
                          }

    // 2. 密封盒解密 (Sealed Box Open)
    // 密文长度必须包含了 overhead 长度，否则就是非法数据
    if (bin_len < crypto_box_SEALBYTES) return "";

    std::vector<unsigned char> decrypted(bin_len - crypto_box_SEALBYTES);

    if (crypto_box_seal_open(decrypted.data(), cipher_bin.data(), bin_len, publickey, secretkey) != 0) {
        // 解密失败（密钥不对或数据被篡改）
        return "";
    }

    // 转为字符串返回
    return string(decrypted.begin(), decrypted.end());
}

bool SimpleRSA::check() const{
    return sizeof publickey == sizeof secretkey;
}

std::string SimpleRSA::encrypt(const std::string &key, const std::string &content) {
    std::vector<unsigned char> pk_bin(crypto_box_PUBLICKEYBYTES);

    // 2. 将 Base64 公钥解码为二进制
    // 如果解码失败（比如公钥格式不对），返回空字符串或抛出异常
    if (sodium_base642bin(pk_bin.data(), crypto_box_PUBLICKEYBYTES,
                          key.c_str(), key.length(),
                          NULL, NULL, NULL, sodium_base64_VARIANT_ORIGINAL) != 0) {
        say("无效的公钥格式");
        return "";
                          }

    // 3. 计算密文长度
    // Libsodium 的密封盒加密，密文长度 = 明文长度 + SEALBYTES (48字节)
    size_t cipher_len = crypto_box_SEALBYTES + content.length();
    std::vector<unsigned char> cipher_bin(cipher_len);

    // 4. 执行加密 (Sealed Box)
    // 只需要：密文容器、明文、明文长度、对方公钥
    if (crypto_box_seal(cipher_bin.data(),
                        (const unsigned char*)content.c_str(), content.length(),
                        pk_bin.data()) != 0) {
        return ""; // 加密失败
                        }

    // 5. 将二进制密文转为 Base64 (方便网络传输)
    size_t b64_len = sodium_base64_ENCODED_LEN(cipher_len, sodium_base64_VARIANT_ORIGINAL);
    std::vector<char> b64_str(b64_len);

    sodium_bin2base64(b64_str.data(), b64_len,
                      cipher_bin.data(), cipher_len,
                      sodium_base64_VARIANT_ORIGINAL);

    return std::string(b64_str.data());
}

SimpleESA::SimpleESA(const string& key) {
    const string& k = getRSA().decrypt(key);
    if (k.length() != crypto_secretbox_KEYBYTES) {
        throwError("Session Key length invalid! Must be 32 bytes.");
    }

    std::memcpy(this -> key, k.data(), crypto_secretbox_KEYBYTES);
}

SimpleESA::SimpleESA(SimpleESA &&other) noexcept {
    std::memcpy(this -> key, other.key, crypto_secretbox_KEYBYTES);
}

std::string SimpleESA::decrypt(const std::string &content) const {
    // 1. Base64 解码
    size_t b64len = content.length();
    // 估算最大长度
    std::vector<unsigned char> cipherBin(b64len);
    size_t binLen = 0;

    if (sodium_base642bin(cipherBin.data(), b64len, content.c_str(), b64len,
                          NULL, &binLen, NULL, sodium_base64_VARIANT_ORIGINAL) != 0) {
        throw std::runtime_error("Invalid Base64 string");
                          }

    // 2. 长度校验
    // 必须要比 Nonce(24) + MAC(16) 长，否则肯定是个坏包
    if (binLen < crypto_secretbox_NONCEBYTES + crypto_secretbox_MACBYTES) {
        return "";
    }

    // 3. 提取 Nonce
    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    std::memcpy(nonce, cipherBin.data(), crypto_secretbox_NONCEBYTES);

    // 4. 提取真正的密文部分
    const unsigned char* cipherText = cipherBin.data() + crypto_secretbox_NONCEBYTES;
    size_t cipherTextLen = binLen - crypto_secretbox_NONCEBYTES;

    // 5. 解密
    std::vector<unsigned char> plain(cipherTextLen - crypto_secretbox_MACBYTES);

    // 如果返回 -1 表示解密失败（可能是遭到篡改，或者 Key 不对）
    if (crypto_secretbox_open_easy(plain.data(), cipherText, cipherTextLen, nonce, key) != 0) {
        // 这里可以记录日志：有人尝试攻击接口
        return "";
    }

    return string(plain.begin(), plain.end());
}

std::string SimpleESA::encrypt(const std::string &content) const {
    // 1. 生成随机 Nonce (24字节)
    // Libsodium 强制要求每次加密必须用不同的 Nonce，否则不安全
    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    randombytes_buf(nonce, sizeof nonce);

    // 2. 加密
    // 密文长度 = 明文长度 + MAC (16字节)
    size_t cipherLen = crypto_secretbox_MACBYTES + content.length();
    std::vector<unsigned char> cipher(cipherLen);

    crypto_secretbox_easy(cipher.data(),
                          (const unsigned char*)content.c_str(), content.length(),
                          nonce, key);

    // 3. 拼接 [Nonce] + [Cipher]
    // 前端需要拿到 Nonce 才能解密，所以要一起发过去
    std::vector<unsigned char> finalData;
    finalData.insert(finalData.end(), nonce, nonce + sizeof nonce);
    finalData.insert(finalData.end(), cipher.begin(), cipher.end());

    // 4. Base64 编码
    size_t b64Len = sodium_base64_ENCODED_LEN(finalData.size(), sodium_base64_VARIANT_ORIGINAL);
    std::vector<char> b64(b64Len);
    sodium_bin2base64(b64.data(), b64Len, finalData.data(), finalData.size(), sodium_base64_VARIANT_ORIGINAL);

    return std::string(b64.data());
}

bool SimpleESA::check() const{
    string content = (char*)key;
    return decrypt(encrypt(content)) == content;
}

Client::Client(const string& key) : esa(key) {
    do {
        unsigned char nonce[16];
        randombytes_buf(nonce, sizeof nonce);
        const size_t b64_len = sodium_base64_ENCODED_LEN(sizeof nonce, sodium_base64_VARIANT_URLSAFE_NO_PADDING);
        std::string id(b64_len, '\0');
        sodium_bin2base64(id.data(), id.size(), nonce, sizeof nonce, sodium_base64_VARIANT_URLSAFE_NO_PADDING);
        id.resize(strlen(id.c_str()));
        ID = id;
    }while (client(ID) != nullptr);
    _handler = nullptr;
    handlers = vector<socialAPI*>();
}

Client::Client(Client &&other) noexcept
: esa(std::move(other.esa)),
handlers(std::move(other.handlers)),
ID(std::move(other.ID)),
_handler(other._handler){}

void Client::getHandler(const std::string& platform, std::shared_ptr<const std::atomic<bool>>& stop){
    if (_handler != nullptr && _handler -> support() == platform)
        return;
    _handler = nullptr;
    socialAPI::instance(&_handler,platform,stop);
    if (_handler != nullptr) {
        _handler -> init();
        handlers.push_back(_handler);
    }
}

bool Client::check() const noexcept{
    return esa.check() && client(ID) == this;
}

const string &Client::getID() const {
    return ID;
}

std::string Client::ESAKey(const std::string &adminKey) const {
    return esa.getKey(adminKey);
}

std::string SimpleESA::getKey(const std::string &adminKey) const {
#ifndef TEST
    if (adminKey == config<string>(ADMIN_CLIENT_KEY)) {
#endif
        char b64[sodium_base64_ENCODED_LEN(crypto_secretbox_KEYBYTES,
            sodium_base64_VARIANT_ORIGINAL)];
        sodium_bin2base64(b64, sizeof b64, key, crypto_secretbox_KEYBYTES,
        sodium_base64_VARIANT_ORIGINAL);
        return std::string(b64);
#ifndef TEST
    }else return "";
#endif
}

namespace {
class AdminLinkedMap final : public LinkedMap<string,Client*> {
public:
    explicit AdminLinkedMap(unsigned int count) : LinkedMap<string,Client*>(count) {}

    pair<string,Client*> put(const string& k,Client* const& v) override {
        for (unsigned int i = 0; i < size(); i++) {
            if (keys[i] == k) {
                auto& old = values[i];
                values[i] = v;
                return {k,old};
            }
        }
        if (_size < maxCount) {
            keys[_size] = k;
            values[_size] = v;
            _size++;
            return {};
        }
        if (hasAdmin) {
            if (_size <= 1) {
                return {};
            }
            auto evicted = pair<string,Client*>(keys[1],values[1]);
            for (unsigned int i = 1; i < maxCount - 1; i++) {
                keys[i] = keys[i + 1];
                values[i] = values[i + 1];
            }
            keys[maxCount - 1] = k;
            values[maxCount - 1] = v;
            return evicted;
        }
        auto evicted = first();
        for (unsigned int i = 0; i < maxCount - 1; i++) {
            keys[i] = keys[i + 1];
            values[i] = values[i + 1];
        }
        keys[maxCount - 1] = k;
        values[maxCount - 1] = v;
        return evicted;
    }

    [[nodiscard]] const Client* getAdminClient() const {
        if (!hasAdmin || _size == 0)
            return nullptr;
        return values[0];
    }

    const Client* markAdmin(const string& id) {
        if (_size == 0)
            return nullptr;
        if (hasAdmin) {
            if (keys[0] == id)
                return values[0];
            for (unsigned int i = 1; i < size(); i++) {
                if (keys[i] == id) {
                    for (unsigned int j = i; j < size() - 1; j++) {
                        keys[j] = keys[j + 1];
                        values[j] = values[j + 1];
                    }
                    _size--;
                    break;
                }
            }
            return values[0];
        }
        for (unsigned int i = 0; i < size(); i++) {
            if (keys[i] == id) {
                if (i == 0) {
                    hasAdmin = true;
                    return values[0];
                }
                const auto adminKey = keys[i];
                const auto adminValue = values[i];
                for (unsigned int j = i; j > 0; j--) {
                    keys[j] = keys[j - 1];
                    values[j] = values[j - 1];
                }
                keys[0] = adminKey;
                values[0] = adminValue;
                hasAdmin = true;
                return values[0];
            }
        }
        return nullptr;
    }

private:
    bool hasAdmin = false;
};
}

AdminLinkedMap* clients;
auto client_mutex = std::mutex();
#define LOCK std::lock_guard<std::mutex> lock(client_mutex);
bool initialized = false;

void Client::init() {
    if (initialized)
        return;
    client_mutex.lock();
    clients = new AdminLinkedMap(config<int>(MAX_CLIENT));
    client_mutex.unlock();
    initialized = true;
}

Client* webAPI::client(const std::string& ID){
    LOCK
    if (clients == nullptr || !clients -> contains(ID))
        return nullptr;
    return (*clients)[ID];
}

bool webAPI::storeClient(Client* client){
    if (client == nullptr || client -> check())
        return false;
    LOCK
    if (clients == nullptr || clients -> contains(client -> getID())) {
        return false;
    }
    clients -> put(client -> getID(),client);
    return clients -> contains(client -> getID());
}

std::string webAPI::createAndStoreClient(const std::string &key) {
    if (const auto client = new Client(key); storeClient(client)) {
        #if TEST
            say("Test Client ID: ",false,GREEN);
            say(client -> getID().c_str(),true,GREEN);
        #endif
        return client -> getID();
    }else delete client;
    #if TEST
        say("Register Client Encounter Problem !");
    #endif
    return "";
}

const Client* webAPI::adminLogin(const string& id,const std::string &adminKey) {
    #ifdef TEST
        LOCK
        return clients -> markAdmin(id);
    #else
    if (adminKey == config<string>(ADMIN_CLIENT_KEY)) {
        LOCK
        return clients -> markAdmin(id);
    }else return nullptr;
    #endif
}

CurlHelper::CurlHelper() {
    curl = curl_easy_init();
}

CurlHelper::~CurlHelper() {
    if(curl != nullptr)
        curl_easy_cleanup(curl);
}

size_t CurlHelper::saveData(char *data, size_t size, size_t member, void *userdata) {
    auto* helper = static_cast<CurlHelper*>(userdata);
    long sizes = size * member;
    helper -> tempData += string(data,sizes);
    return sizes;
}

void CurlHelper::curlSetup() {
    if(curl == nullptr){
        throwError("创建CURL失败");
        return;
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlHelper::saveData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
}

bool CurlHelper::connect(bool deal) {
    curl_easy_setopt(curl,CURLOPT_URL,nextURL().c_str());
    CURLcode code = curl_easy_perform(curl);
    if (CURLE_OK != code) {
        clear();
        warn("连接链接失败，信息如下：", false);
        warn(curl_easy_strerror(code), false);
        warn("错误码：", false);
        warn(to_string(code).c_str());
        return false;
    }
    return deal ? dealJson() : true;
}

bool AutoCurlHelper::dealJson() {
    #ifdef DEVELOP
        try {
    #endif
            json = Json::parse(tempData);
    #ifdef DEVELOP
        }catch (std::exception& e) {
            warn("爬取数据格式错误！");
            warn("数据如下：");
            warn(tempData.c_str());
            throwError(e.what());
        }
    #endif
    return deal(json);
}
