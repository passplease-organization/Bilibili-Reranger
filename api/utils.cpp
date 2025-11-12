#include <filesystem>
#include <iostream>
#include <fstream>
#include "Util.h"

using namespace std;
using Json = nlohmann::json;
namespace fs = std::filesystem;

const string ConfigPath = "config";
auto storedJson = map<string,Json>();

bool endWith(const char* target,const char* substring){
    auto t = string(target);
    auto s = string(substring);
    if(t.size() < s.size())
        return false;
    return t.substr(t.size() - s.size()) == s;
}

bool startWith(const char* target,const char* substring){
    string t = string(target);
    string s = string(substring);
    if(t.size() < s.size())
        return false;
    return t.substr(0, s.size()) == s;
}

string removeEnd(const string& target,const string& substring){
    return endWith(target.c_str(),substring.c_str()) ? target.substr(0,target.size() - substring.size()) : target;
}

int removeEnd(const char* target,const char* substring,char** buffer){
    string t = string(target);
    string s = string(substring);
    string back = removeEnd(t,s);
    defaultOutputChar(buffer);
    return snprintf(*buffer,MAX_BUFFER_SIZE,"%s",back.c_str());
}

bool fileExists(const char* name, bool absolute){
    return name != nullptr && (absolute ? fs::exists(fs::absolute(name)) : fs::exists(name));
}

void throwError(const char* error) noexcept(false){
    cout << RED << error << RESET << endl;
    throw runtime_error(error);
}

void say(const char* message,bool endl,const char* color){
    if(color != nullptr)
        cout << string(color);
    cout << message << RESET;
    if(endl)
        cout << '\n';
}

inline void warn(const char* warn,bool endl){
    say(warn,endl,YELLOW);
}

inline bool convertToInt(const char* str, int& num){
    istringstream stream(str);
    stream >> num;
    return stream.fail();
}

Json getJson(const char* name, const char* path){
    if(fileExists(path)){
        Json json;
        ifstream file(path);
        file >> json;
        return json;
    }
    if(name != nullptr){
        auto json = storedJson.find(name);
        if(json != storedJson.end())
            return json -> second;
    }
    return nullptr;
}

inline void defaultOutputChar(char** output){
    *output = new char[MAX_BUFFER_SIZE];
}

inline void freeOutputChar(char** output){
    delete[] *output;
}

void deleteConfig(const char* filePath,bool absolute, const char* fileType){
    char* back;
    defaultOutputChar(&back);
    if(absolute){
        snprintf(back,MAX_BUFFER_SIZE,"%s",filePath);
    }else toConfigPath(back,filePath,MAX_BUFFER_SIZE, fileType);
    string path(back);
    freeOutputChar(&back);
    remove(path.c_str());
}

bool createConfig(char* output, const char* filePath,const size_t maxLength,const char* fileType){
    try{
        if(!fileExists(ConfigPath.c_str())){
            filesystem::create_directory(ConfigPath);
        }
        char* back;
        defaultOutputChar(&back);
        toConfigPath(back,filePath,MAX_BUFFER_SIZE, fileType);
        string path(back);
        freeOutputChar(&back);
        ofstream file(path.c_str());
        if(file.is_open()){
            file << "{}";
            file.close();
            snprintf(output,maxLength,"%s",path.c_str());
            return true;
        }else {
            warn("创建文件失败！请检查具体原因！");
            throwError("Failed at creating file !");
        }
        return false;
    }catch (...){
        return false;
    }
}

bool getConfig(char* output, const char* filePath,const size_t maxLength, const char* fileType){
    try{
        string path;
        if(startWith(filePath,ConfigPath.c_str())){
            path = filePath;
        }else {
            char* back = nullptr;
            defaultOutputChar(&back);
            toConfigPath(back,filePath,MAX_BUFFER_SIZE, fileType);
            path = string(back);
            freeOutputChar(&back);
        }
        if(fileExists(path.c_str())) {
            snprintf(output,maxLength,"%s",path.c_str());
            return true;
        }
        char* out;
        defaultOutputChar(&out);
        if(createConfig(out,filePath,MAX_BUFFER_SIZE,fileType) && fileExists(path.c_str())) {
            snprintf(output,maxLength,"%s",out);
            freeOutputChar(&out);
            return true;
        }
        freeOutputChar(&out);
        throwError("Failed at creating file !");
        return false;
    }catch (...){
        return false;
    }
}

void toConfigPath(char* back, const char* filePath,const size_t maxLength, const char* fileType){
    string path{};
    #ifdef WIN32
        path = path.append(ConfigPath).append("\\").append(filePath).append(fileType);
    #elifdef __linux__
        path = path.append(ConfigPath).append("/").append(filePath).append(fileType);
    #endif
    snprintf(back,maxLength,"%s",path.c_str());
}

bool saveToFile(const char* name,const char* path,bool recover){
    auto json = getJson(name,nullptr);
    if(json == nullptr)
        json = getJson(name,path);
    if(json != nullptr){
        const auto& dir = fs::path(path).parent_path();
        if(!dir.empty() && !fs::exists(dir)){
            fs::create_directories(dir);
        }
        ofstream file(path,ios::out);
        if(recover)
            file.clear();
        file << json;
        file.close();
        return true;
    }else return false;
}

bool storeJson(const char* name,const char* path,const Json& json,bool release){
    if(release){
        storedJson.erase(name);
        return !storedJson.contains(name);
    }else {
        storedJson[name] = json == nullptr ? getJson(name,path) : json;
        return storedJson.contains(name);
    }
}

namespace dataStore{

    Data::Data(bool valid) {
        this -> _valid = valid;
        saved = true;
    }

    Data::Data(const dataStore::Data &other) {
        if (other.valid()) {
            *this = other;
            this -> _valid = true;
        }
    }

    Data::Data(const dataStore::Data* other){
        if (other -> valid()) {
            *this = other;
            this -> _valid = true;
        }
    }

    Data::Data(dataStore::Data&& old) noexcept{
        if (old.valid()) {
            *this = old;
            old.clear();
            this -> _valid = true;
        }
    }

    Data::~Data(){
        if(needSave())
            writeToJson();
        clear();
        broken();
    }

    void Data::clear(){
        data.clear();
        dataArrays.clear();
        strings.clear();
        stringArrays.clear();
        ints.clear();
        intArrays.clear();
        floats.clear();
        floatArrays.clear();
        bools.clear();
        boolArrays.clear();
        _valid = true;
        saved = false;
    }

    bool Data::valid() const {
        return _valid;
    }

    void Data::broken() {
        this -> _valid = false;
    }

    void Data::setName(const char *_name, bool force) {
        if(this -> name.empty() || force) {
            this->name = string(_name);
            saved = false;
        }
    }

    void Data::setPath(const char *_path, bool force) {
        if(this -> path.empty() || force) {
            char* o;
            defaultOutputChar(&o);
            getConfig(o, _path);
            this -> path = o;
            saved = false;
        }
    }

    Data &Data::operator=(const dataStore::Data *other) {
        if(valid() && other != nullptr && other -> valid()) {
            this->data = other->data;
            this->dataArrays = other->dataArrays;
            this->strings = other->strings;
            this->stringArrays = other->stringArrays;
            this->ints = other->ints;
            this->intArrays = other->intArrays;
            this->floats = other->floats;
            this->floatArrays = other->floatArrays;
            this->bools = other->bools;
            this->boolArrays = other->boolArrays;
            this->saved = false;
            this->_neverSave = other->_neverSave;
        }
        return *this;
    }

    Data &Data::operator=(const dataStore::Data &other) {
        if(other.valid() && &other != this) {
            this->data = other.data;
            this->dataArrays = other.dataArrays;
            this->strings = other.strings;
            this->stringArrays = other.stringArrays;
            this->ints = other.ints;
            this->intArrays = other.intArrays;
            this->floats = other.floats;
            this->floatArrays = other.floatArrays;
            this->bools = other.bools;
            this->boolArrays = other.boolArrays;
            this->saved = false;
            this->_neverSave = other._neverSave;
        }
        return *this;
    }

    bool Data::operator==(const dataStore::Data& other) const {
        if(this == &other)
            return true;
        if(
                valid() && other.valid()
                && strings == other.strings
                && stringArrays == other.stringArrays
                && ints == other.ints
                && intArrays == other.intArrays
                && floats == other.floats
                && floatArrays == other.floatArrays
                && bools == other.bools
                && boolArrays == other.boolArrays
                ){
            for(const pair<const string, Data>& d : other.data){
                if(d.second == data.at(d.first))
                    continue;
                return false;
            }
            for(const pair<const string,vector<Data>>& _d : other.dataArrays){
                if(this -> dataArrays.contains(_d.first)){
                    const auto d = this -> dataArrays.at(_d.first);
                    if(d.size() == _d.second.size()){
                        for(int i = 0;i < d.size();i++){
                            if(d.at(i) == _d.second.at(i)){
                                continue;
                            }
                            return false;
                        }
                        continue;
                    }
                }
                return false;
            }
            return true;
        }else return false;
    }

    bool Data::operator!=(const dataStore::Data& other) const{
        return !(*this == other);
    }

    Data *Data::operator+=(const dataStore::Data *other) {
        if(valid() && other != nullptr && other -> valid()) {
            data.insert(other->data.begin(), other->data.end());
            for(const auto& temp : other -> dataArrays){
                dataStore::put<Data>(this,temp.first.c_str(),&(temp.second));
            }
            strings.insert(other->strings.begin(), other->strings.end());
            for(const auto& temp : other -> stringArrays){
                dataStore::put<string>(this,temp.first.c_str(),&(temp.second));
            }
            ints.insert(other->ints.begin(), other->ints.end());
            for(const auto& temp : other -> intArrays){
                dataStore::put<int>(this,temp.first.c_str(),&(temp.second));
            }
            floats.insert(other->floats.begin(), other->floats.end());
            for(const auto& temp : other -> floatArrays){
                dataStore::put<float>(this,temp.first.c_str(),&(temp.second));
            }
            bools.insert(other->bools.begin(), other->bools.end());
            for(const auto& temp : other -> boolArrays){
                dataStore::put<bool>(this,temp.first.c_str(),&(temp.second));
            }
            saved = false;
        }
        return this;
    }

    Data* Data::operator+=(const Data& other){
        return *this += &other;
    }

    Data Data::operator+(const dataStore::Data *other) const {
        Data value{};
        if(valid() && other -> valid()) {
            value += this;
            value += other;
        }
        return value;
    }

    void Data::copy(Data** a) const {
        *a = new Data{};
        **a = this;
    }

    bool Data::needSave() const{
        return !neverSave() && !saved && valid();
    }

    bool Data::neverSave() const {
        return _neverSave || name.empty() || path.empty();
    }

    void Data::NeverSave() {
        _neverSave = true;
    }

    inline void setSaved(dataStore::Data* data, bool saved = false){
        data -> saved = saved;
    }

    template<typename T>
    map<const string,T>* getMap(Data* data){
        if constexpr (is_same_v<T,Data>)
            return &(data -> data);
        else if constexpr (is_same_v<T,string>)
            return &(data -> strings);
        else if constexpr (is_same_v<T,int>)
            return &(data -> ints);
        else if constexpr (is_same_v<T,float>)
            return &(data -> floats);
        else if constexpr (is_same_v<T,bool>)
            return &(data -> bools);
        throwError("Wrong Type !");
    }

    template<typename T>
    map<const string,vector<T>>* getVector(Data* data){
        if constexpr (is_same_v<T,Data>)
            return &(data -> dataArrays);
        else if constexpr (is_same_v<T,string>)
            return &(data -> stringArrays);
        else if constexpr (is_same_v<T,int>)
            return &(data -> intArrays);
        else if constexpr (is_same_v<T,float>)
            return &(data -> floatArrays);
        else if constexpr (is_same_v<T,bool>)
            return &(data -> boolArrays);
        throwError("Wrong Type !");
    }

    template<typename T>
    void put(Data* data,const char* label,const T *content, bool vector, bool recover){
        if(!data -> valid())
            return;
        string stringLabel(label);
        if(vector){
            auto v = getVector<T>(data);
            if(v -> contains(stringLabel)){
                if(content != nullptr)
                    (*v)[stringLabel].emplace_back(*content);
                else v -> erase(stringLabel);
                setSaved(data);
            }else if(content != nullptr){
                std::vector<T> newVector{};
                newVector.emplace_back(*content);
                (*v)[stringLabel] = newVector;
                setSaved(data);
            }
        }else{
            auto m = getMap<T>(data);
            if(m -> contains(stringLabel)){
                if(content == nullptr){
                    m -> erase(stringLabel);
                    setSaved(data);
                    return;
                }
                if(recover) {
                    (*m)[stringLabel] = *content;
                    setSaved(data);
                }
            }else if(content != nullptr) {
                (*m)[stringLabel] = *content;
                setSaved(data);
            }
        }
    }

    template<typename T>
    void put(Data* data,const char* label,const vector<T> *content){
        auto vector = getVector<T>(data);
        string l(label);
        if(vector->contains(l)) {
            auto& v = (*vector)[l];
            v.insert(v.end(),content->begin(), content->end());
        }else (*vector)[l] = *content;
    }

    void Data::validData(const dataStore::Data *input) noexcept(false) {
        if(!valid())
            return;
        for(const auto& d : input -> data){
            if(&d.second == this){
                throwError("Any json file cannot contain itself !");
                return;
            }
        }
        for(const auto& dArray : input -> dataArrays){
            for(const auto& d : dArray.second){
                if(&d == this){
                    throwError("Any json file cannot contain itself !");
                    return;
                }
            }
        }
    }

    void Data::put(const char *label, const dataStore::Data *content, bool vector, bool recover) {
        validData(content);
        dataStore::put<Data>(this,label,content,vector,recover);
    }

    void Data::put(const char *label, const char *content, bool vector, bool recover) {
        string s(content);
        dataStore::put<string>(this,label,&s,vector,recover);
    }

    void Data::put(const char *label, const int *content, bool vector, bool recover) {
        dataStore::put<int>(this,label,content,vector,recover);
    }

    void Data::put(const char *label, const float *content, bool vector, bool recover){
        dataStore::put<float>(this,label,content,vector,recover);
    }

    void Data::put(const char *label, const bool *content, bool vector, bool recover){
        dataStore::put<bool>(this,label,content,vector,recover);
    }

    void Data::put(const char *label, const dataStore::Data &content, bool vector, bool recover) {
        validData(&content);
        dataStore::put<Data>(this,label,&content,vector,recover);
    }

    void Data::put(const char *label, const int &content, bool vector, bool recover) {
        dataStore::put<int>(this,label,&content,vector,recover);
    }

    void Data::put(const char *label, const float &content, bool vector, bool recover){
        dataStore::put<float>(this,label,&content,vector,recover);
    }

    void Data::put(const char *label, const bool &content, bool vector, bool recover){
        dataStore::put<bool>(this,label,&content,vector,recover);
    }

    template<typename T>
    void getMap(Data* data,const char *label,T** input,const bool copy){
        auto m = dataStore::getMap<T>(data);
        if(copy)
            **input = m -> at(label);
        else *input = &(m -> at(label));
    }

    template<typename T>
    void getVector(Data* data,const char *label,vector<T>** input,const bool copy){
        auto v = dataStore::getVector<T>(data);
        if(copy)
            **input = v -> at(label);
        else *input = &(v -> at(label));
    }

    void Data::get(const char *label, dataStore::Data **data,const bool copy) {
        dataStore::getMap<Data>(this,label,data,copy);
    }

    void Data::get(const char *label, vector<dataStore::Data> **data,const bool copy) {
        dataStore::getVector<Data>(this,label,data,copy);
    }

    void Data::get(const char *label,const char **string, bool) const{
        *string = this -> strings.at(label).c_str();
    }

    void Data::get(const char *label, vector<const char *> **string,const bool copy) const {
        if(string == nullptr)
            return;
        auto s = this -> stringArrays.at(label);
        for(auto element : s){
            if(copy)
                element = std::string(element);
            (*string) -> emplace_back(element.c_str());
        }
    }

    void Data::get(const char *label, int **ints, bool copy) {
        dataStore::getMap<int>(this,label,ints,copy);
    }

    void Data::get(const char *label, vector<int> **ints, bool copy) {
        dataStore::getVector<int>(this,label,ints,copy);
    }

    void Data::get(const char *label, float **floats, bool copy) {
        dataStore::getMap<float>(this,label,floats,copy);
    }

    void Data::get(const char *label, vector<float> **floats, bool copy) {
        dataStore::getVector<float>(this,label,floats,copy);
    }

    void Data::get(const char *label, bool **bools, bool copy){
        dataStore::getMap<bool>(this,label,bools,copy);
    }

    void Data::get(const char *label, vector<bool> **bools, bool copy){
        dataStore::getVector<bool>(this,label,bools,copy);
    }

    bool Data::empty(){
        return data.empty() && dataArrays.empty() && strings.empty() && stringArrays.empty() && ints.empty() && intArrays.empty() && floats.empty() && floatArrays.empty() && bools.empty() && boolArrays.empty();
    }

    bool Data::writeToJson(const char *target_name, const char *target_path,bool recover, bool storage) {
        if(!needSave())
            return false;
        setName(target_name);
        setPath(target_path);
        Json json = recover ? Json() : getJson(name.c_str(), path.c_str());
        if (!recover && json == nullptr) {
            string error = "File not exists ! Cannot write Json ! File path: ";
            error.append(path);
            throwError(error.c_str());
            return false;
        }else {
            dataStore::to_json(json,*this);
            storeJson(name.c_str(),path.c_str(),json);
            bool back = !storage || saveToFile(target_name, target_path);
            saved = back;
            return back;
        }
    }

    bool Data::writeToJson() {
        return !this -> name.empty() && !this -> path.empty() && writeToJson(this -> name.c_str(),this -> path.c_str());
    }

    Data Data::readFromJson(const char *path, const char *name, const bool _throw) {
        Data d;
        d.setName(name);
        ifstream file;
        char* filePath;
        if(fileExists(path)){
            d.setPath(path);
            file = ifstream(path);
        }else {
            defaultOutputChar(&filePath);
            d = Data{};
            if(!getConfig(filePath,path)) {
                if(_throw) {
                    string error("Path not exists ! Current path: ");
                    error.append(path);
                    throwError(error.c_str());
                }
                d.broken();
                return d;
            }
            d.setPath(filePath);
            file = ifstream(filePath);
        }
        if(file.is_open()) {
            Json json;
            file >> json;
            file.close();
            d = json.get<dataStore::Data>();
        }else d.broken();
        return d;
    }

    void from_json(const Json& json,Data& data){
        try{
            data.clear();
            for(const auto& pair : json.items()){
                const Json& j = pair.value();
                const string& label = pair.key();
                if(j.is_array()){
                    if(j.empty())
                        continue;
                    switch (j[0].type()) {
                        case nlohmann::detail::value_t::object: {
                            for(int slot = 0;slot < j.size();slot++) {
                                const Data dataInSlot = j[slot].get<Data>();
                                data.put(label.c_str(),&dataInSlot,true);
                            }
                            break;
                        }
                        case nlohmann::detail::value_t::string: {
                            for(int slot = 0;slot < j.size();slot++) {
                                const string dataInSlot = j[slot].get<string>();
                                data.put(label.c_str(),dataInSlot.c_str(),true);
                            }
                            break;
                        }
                        case nlohmann::detail::value_t::number_unsigned:
                        case nlohmann::detail::value_t::number_integer: {
                            for(int slot = 0;slot < j.size();slot++) {
                                const int dataInSlot = j[slot].get<int>();
                                data.put(label.c_str(),&dataInSlot,true);
                            }
                            break;
                        }
                        case nlohmann::detail::value_t::number_float: {
                            for(int slot = 0;slot < j.size();slot++) {
                                const float dataInSlot = j[slot].get<float>();
                                data.put(label.c_str(),&dataInSlot,true);
                            }
                            break;
                        }
                        case nlohmann::detail::value_t::boolean: {
                            for(int slot = 0;slot < j.size();slot++) {
                                const bool dataInSlot = j[slot].get<bool>();
                                data.put(label.c_str(),&dataInSlot,true);
                            }
                        }
                        case nlohmann::detail::value_t::null:
                            break;
                        default: {
                            data.broken();
                            string error("Unchecked Json file, not right format. now label: ");
                            error += label;
                            throwError(error.c_str());
                            return;
                        }
                    }
                }else {
                    switch (j.type()) {
                        case nlohmann::detail::value_t::object: {
                            const Data d = j.get<Data>();
                            data.put(label.c_str(),&d);
                            break;
                        }
                        case nlohmann::detail::value_t::string: {
                            const string d = j.get<string>();
                            data.put(label.c_str(),d.c_str());
                            break;
                        }
                        case nlohmann::detail::value_t::number_unsigned:
                        case nlohmann::detail::value_t::number_integer: {
                            const int d = j.get<int>();
                            data.put(label.c_str(),&d);
                            break;
                        }
                        case nlohmann::detail::value_t::number_float: {
                            const float d = j.get<float>();
                            data.put(label.c_str(),&d);
                            break;
                        }
                        case nlohmann::detail::value_t::boolean: {
                            const bool d = j.get<bool>();
                            data.put(label.c_str(),&d);
                            break;
                        }
                        case nlohmann::detail::value_t::null:
                            break;
                        default: {
                            data.broken();
                            string error("Unchecked Json file, not right format. now label: ");
                            error += label;
                            throwError(error.c_str());
                            return;
                        }
                    }
                }
            }
        }catch(const std::exception& e) {
            data.broken();
            say("Invalid Json File !",false,RED);
            if(!data.path.empty())
                say(("File Path: " + data.path).c_str(),false, RED);
            say("Exception Info: ",false,BLUE);
            say(e.what(),true,BLUE);
            throwError(e.what());
        }
    }

    void to_json(Json& json,const Data& data){
        string label;
        for(const auto& i : data.data){
            label = json.contains(i.first) ? Data::DATA + i.first : i.first;
            json[label] = i.second;
        }
        for(const auto& i : data.dataArrays){
            label = json.contains(i.first) ? Data::DATA_ARRAY + i.first : i.first;
            for(int slot = 0;slot < i.second.size(); slot++)
                json[label][slot] = i.second.at(slot);
        }
        for(const auto& i : data.strings){
            label = json.contains(i.first) ? Data::STRING + i.first : i.first;
            json[label] = i.second;
        }
        for(const auto& i : data.stringArrays){
            label = json.contains(i.first) ? Data::STRING_ARRAY + i.first : i.first;
            for(int slot = 0;slot < i.second.size(); slot++)
                json[label][slot] = i.second.at(slot);
        }
        for(const auto& i : data.ints){
            label = json.contains(i.first) ? Data::INT + i.first : i.first;
            json[label] = i.second;
        }
        for(const auto& i : data.intArrays){
            label = json.contains(i.first) ? Data::INT_ARRAY + i.first : i.first;
            for(int slot = 0;slot < i.second.size(); slot++)
                json[label][slot] = i.second.at(slot);
        }
        for(const auto& i : data.floats){
            label = json.contains(i.first) ? Data::FLOAT + i.first : i.first;
            json[label] = i.second;
        }
        for(const auto& i : data.floatArrays){
            label = json.contains(i.first) ? Data::FLOAT_ARRAY + i.first : i.first;
            for(int slot = 0;slot < i.second.size(); slot++)
                json[label][slot] = i.second.at(slot);
        }
        for(const auto& i : data.bools){
            label = json.contains(i.first) ? Data::BOOL + i.first : i.first;
            json[label] = i.second;
        }
        for(const auto& i : data.boolArrays){
            label = json.contains(i.first) ? Data::BOOL_ARRAY + i.first : i.first;
            for(int slot = 0;slot < i.second.size(); slot++)
                json[label][slot] = i.second.at(slot);
        }
    }
}

const string dataStore::Data::DATA = "data_";
const string dataStore::Data::DATA_ARRAY = "data_array_";
const string dataStore::Data::STRING = "string_";
const string dataStore::Data::STRING_ARRAY = "string_array_";
const string dataStore::Data::INT = "int_";
const string dataStore::Data::INT_ARRAY = "int_array_";
const string dataStore::Data::FLOAT = "float_";
const string dataStore::Data::FLOAT_ARRAY = "float_array_";
const string dataStore::Data::BOOL = "bool_";
const string dataStore::Data::BOOL_ARRAY = "bool_array_";

template void dataStore::getMap<string>(dataStore::Data*,const char *,string** ,bool);