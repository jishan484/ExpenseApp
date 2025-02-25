#include <iostream>
#include <vector>
#include <variant>
#include <sstream>
#include <string>
#include <functional>

class JSON;

using namespace std;
using String = string;


/// @brief helps to create JSON obj visually at ease
class JSON
{
protected:
    struct TRACKER_
    {
        bool isArray = 0;
        bool isOpened = 0;
        bool isFirstKey = 1;
        bool isLoop = 0;
        TRACKER_(bool isArray, bool isOpened, bool isFirstKey, bool isLoop = 0) {
            this->isArray = isArray;
            this->isOpened = isOpened;
            this->isFirstKey = isFirstKey;
            this->isLoop = isLoop;
        }
    };
    struct JSON_Syntax_Exception{
        // need to implement
    };
private:
    std::ostringstream buffer;
    std::string tab = "";
    std::vector<TRACKER_> flow; // 0 for obj , 1 for array [it tracks current and previous types]
    
    
public:
using JSONValue = std::variant<
        std::string,
        int,
        float,
        char,
        char*,
        const char *,
        double,
        bool,
        JSON*>;
    // marker(s)
    struct ARRAY_{
        JSON* json;
        JSON& operator<<(const char* value){
            json->buffer << "[\n";
            json->tab.append("    ");
            json->buffer << json->tab << '\"' << value << '\"';
            json->flow.push_back(TRACKER_(1, 1, 0));
            return *json;
        }
        static std::function<std::vector<JSON::JSONValue>()> attach;
        static std::function<JSON::JSONValue(int)> loop;
        static std::function<void(int, bool, int)> forEach;
        static std::vector<JSON::JSONValue> buffer;
        public:
        #define forEach(a, b, c) JSON::ARRAY_::attach = [](){ \
                                for(a; b; ) JSON::ARRAY_::buffer.push_back(JSON::ARRAY_::loop(c)); return JSON::ARRAY_::buffer; \
                              }; JSON::ARRAY_::loop = [&](a)->JSON::JSONValue
    };
    ARRAY_ ARRAY;
    JSON(){
        ARRAY.json = this;
    }
    ~JSON() { std::cout << "JSON CLOSED" << std::endl; }
    String toString()
    {
        return buffer.str();
    }

    /// @brief for obj key and `,` (key will be always string and contains `:`)
    /// @param key string
    /// @return JSON
    JSON& operator<<(const char* key){
        if(!flow.empty() && flow.back().isLoop){
            if(flow.back().isFirstKey){
                buffer << "[\n";
                flow.back().isFirstKey = false;
            } else {
                buffer << ",\n";
            }
            buffer << tab << "{\n";
            tab.append("    ");
            buffer << tab << '\"' << key << "\": ";
            flow.push_back(TRACKER_(0,1,0));
            return *this;
        }
        if(!flow.empty() && flow.back().isArray){
            if(!flow.back().isFirstKey) buffer << '\"';
            buffer << '\n' << tab << '\"' << key << '\"';
            flow.back().isFirstKey = 0;
            return *this;
        }
        if(flow.empty()){
            buffer << tab << "{\n";
            tab.append("    ");
            buffer << tab << '\"' << key << "\": ";
            flow.push_back(TRACKER_(0,1,0));
        } else {
            if(flow.back().isFirstKey) {
                buffer << "\n" << tab << '\"' << key << "\": ";
                flow.back().isFirstKey = 0;
            } else {
                buffer << ",\n" << tab << '\"' << key << "\": ";
            }
        }
        return *this;
    }

    /// @brief closes current JSON obj or opens a new JSON obj inside array [based on flow]
    /// @param key JSON
    /// @return JSON
    JSON& operator<<(JSON& json){
        // for obj it closes
        if(!flow.back().isArray){
            tab.erase(tab.length()-4,4);
            buffer << '\n' << tab << '}';
            flow.pop_back();
        } else {
            buffer << ",\n" << tab << "{";
            tab.append("    ");
            flow.push_back(TRACKER_(0,1,1));
        }
        return *this;
    }

    //lookit
    JSON& operator<<(JSON::ARRAY_& jsonArray){
        if(flow.back().isLoop && !JSON::ARRAY_::buffer.empty()){
            JSON::ARRAY_::attach();
            buffer << '[';
            bool is_frst_entry = true;
                for(auto &data : JSON::ARRAY_::buffer){
                    if(!is_frst_entry) buffer << ',';
                    is_frst_entry = false;
                    buffer << '\n' << tab;
                    if (std::string* str = std::get_if<std::string>(&data)) {
                        buffer << *str;
                    } else if (double* str = std::get_if<double>(&data)) {
                        buffer << *str;
                    } else if (bool* str = std::get_if<bool>(&data)) {
                        buffer << *str;
                    } else if (auto str = std::get_if<JSON*>(&data)) {
                    }
                }
        } else if(flow.back().isLoop){
            JSON::ARRAY_::attach();
        }
        // for obj it closes
        if(flow.back().isArray){
            tab.erase(tab.length()-4,4);
            buffer << '\n' << tab << ']';
            flow.pop_back();
        }
        return *this;
    }

    /// @brief for value [string]
    /// @param key string, obj, array, int, float, double, bool
    /// @return JSON
    JSON& operator>>(const char* value){
        if(!flow.back().isArray)
            buffer << '\"' << value << "\"";
        else {
            if(flow.back().isFirstKey){
                buffer << '\n' << tab << '\"' << value << "\"";
                flow.back().isFirstKey = 0;
            }
            else
                buffer << ',' << '\n' << tab << '\"' << value << "\"";
        }
        return *this;
    }
    /// @brief for value [int, float, double, bool]
    /// @param key string, obj, array, int, float, double, bool
    /// @return JSON
    JSON& operator>>(double value){
        if(!flow.back().isArray)
            buffer << value;
        else {
            buffer << tab << value;
        }
        return *this;
    }
    /// @brief for value [JSON obj]
    /// @param key string, obj, array, int, float, double, bool
    /// @return JSON
    JSON& operator>>(JSON& jaon){
        if(flow.back().isFirstKey){
            buffer << '\n' << tab << '{';
            flow.back().isFirstKey = 0;
        } else {
            if(flow.back().isArray){
                buffer << ",\n" << tab << '{';
            } else {
                buffer << '{';
            }
        }
        tab.append("    ");
        flow.push_back(TRACKER_(0, 1, 1));
        return *this;
    }
    /// @brief for value [JSON Array]
    /// @param key string, obj, array, int, float, double, bool
    /// @return JSON
    JSON& operator>>(ARRAY_& jaon){
        if(flow.back().isArray && !flow.back().isFirstKey) buffer << ',';
        if(!flow.back().isArray) {
            buffer << '[';
        } else {
            buffer << '\n' << tab << '[';
        }
        tab.append("    ");
        flow.push_back(TRACKER_(1, 1, 1));
        return *this;
    }

    std::function<std::vector<JSONValue>()>& operator>>(std::function<std::vector<JSONValue>()>& jaon){
        // reset ARRAY_::buffer
        ARRAY_::buffer.clear();
        tab.append("    ");
        flow.push_back(TRACKER_(1,0,1,1));
        return this->ARRAY.attach;
    }

    /// @brief prints in cout
    /// @param os 
    /// @param obj 
    /// @return ostream
    friend std::ostream& operator<<(std::ostream& os, const JSON& obj) {
        os << obj.buffer.str();
        return os;
    }
};
std::function<std::vector<JSON::JSONValue>()> JSON::ARRAY_::attach;
std::function<JSON::JSONValue(int)> JSON::ARRAY_::loop;
std::vector<JSON::JSONValue> JSON::ARRAY_::buffer;


void tests();
void l();

int main()
{
    // tests();
    // l();
    JSON json;
    json <<
        "aa" >>
            json.ARRAY.forEach(int i=0,i<3,i++){
                return &(json << "a" >> "b" <<json);
            };
        json << json.ARRAY <<
        "bb" >> "123" <<
        "cc" >>
            json.ARRAY.forEach(int i=0,i<3,i++){
                return &(json << "ports" >> i <<json);
            };
        json << json.ARRAY
    <<json;

    cout << json << endl;

    return 0;
}

void l(){
    JSON json;
    json <<
        "type" >> "network info" <<
        "ports" >> 
            json <<
                "8080" >> "open" <<
                "8443" >> "open"
            <<json <<
            "processes" >>
                json.ARRAY >>
                    "100" >>
                    "101" >>
                    "110" >>
                    json.ARRAY >>
                        "T100" >>
                        "B101" >>
                        "C110"
                    << json.ARRAY >>
                    json << 
                        "abs" >> "on"
                    << json
                << json.ARRAY <<
        "ports_info" >>
            json.ARRAY >>
                json <<
                    "type" >> "b"
                <<json <<
                json <<
                    "type" >> "b"
                <<json
            << json.ARRAY
    << json;
    cout <<json.toString() << endl;
}


// void test___2();

// void tt(){
//     JSON json;
//     json.ARRAY >> //this [ and value
//         "a" >>
//         "b" >>
//         "c" >>
//         json.ARRAY >>
//             // json >>
//             //     "qq" >> "vv"
//             // << json >>
//             "aa" >>
//             2
//         << json.ARRAY 
//         >>
//         json >>
//             "ook" >> 1 <<
//             "aa" >> "1" <<
//             "ports" >>
//             json.ARRAY >> //this [ and value
//                 "a" >>  //this value and ,
//                 "b"
//             << json.ARRAY
//         << json //lllo
//     << json.ARRAY;
//     cout << json.toString() << endl;
// }





// // hardcoded values
// void test___2()
// {
//     JSON json;

//     json.ARRAY >>
        
//         json >> 
//             "type" >> "http" << 
//             "is_open" >> true
//         << json >>
//         "8080" 
//     << json.ARRAY;
//     std::cout<< json.toString() <<endl;
// }


// void test__2()
// {
//     JSON json;

//     json.ARRAY >>
//         json >> 
//             "8080" >>
//             json >> 
//                 "type" >> "http" << 
//                 "is_open" >> true
//             << json
//         <<json >>
//         json >> 
//             "8443" >>
//             json >> 
//                 "type" >> "https" << 
//                 "is_open" >> false
//             << json
//         <<json
//     << json.ARRAY;
//     std::cout<< json.toString() <<endl;
// }



// test 1(single json key value pair)
void test1(){
    String expect = "{\n    \"a\": \"a\"\n}";
    JSON json;
    json << 
        "a" >> "a"
    << json;
    String result = json.toString();
    if(result == expect){
        cout<< "test 1 [simeple json obj] : passed \n" << expect <<endl;
    } else {
        cout<< "test 1 [simeple json obj] : faild"<<endl;
        cout << ":" << result << ":"<< result.length() << endl;
        cout << ":" << expect << ":" << expect.length() << endl;
    }
}

// test 2 (a normal json obj)
void test2(){
    String expect = "{\n    \"a\": \"a\",\n    \"b\": 2,\n    \"c\": 3.2\n}";
    JSON json;
    json <<
        "a" >> "a" <<
        "b" >> 2 <<
        "c" >> 3.2
    << json;
    String result = json.toString();
    if(result == expect){
        cout<< "test 2 [simeple json obj] : passed \n" << expect <<endl;
    } else {
        cout<< "test 2 [simeple json obj] : faild"<<endl;
        cout << ":" << result << ":"<< result.length() << endl;
        cout << ":" << expect << ":" << expect.length() << endl;
    }
}

// test 3 (a nested json obj)
void test3(){
    String expect = "{\n    \"a\": \"a\",\n    \"b\": {\n        \"aa\": \"aa\",\n        \"bb\": 6,\n        \"cc\": 2.2\n    },\n    \"c\": 3.2\n}";
    JSON json;
    json << 
        "a" >> "a" <<
        "b" >> 
            json <<
                "aa" >> "aa" <<
                "bb" >> 6 <<
                "cc" >> 2.2
            << json <<
        "c" >> 3.2
    << json;
    String result = json.toString();
    if(result == expect){
        cout<< "test 3 [nested json obj] : passed \n" << expect <<endl;
    } else {
        cout<< "test 3 [nested json obj] : faild"<<endl;
        cout << ":" << result << ":"<< result.length() << endl;
        cout << ":" << expect << ":" << expect.length() << endl;
    }
}

// test 4 (nested json objs)
void test4(){
    String expect = "{\n    \"a\": \"a\",\n    \"b\": {\n        \"aa\": \"aa\",\n        \"bb\": 6,\n        \"cc\": 2.2\n    },\n    \"c\": {\n        \"ab\": \"ab\",\n        \"bd\": 0,\n        \"gc\": 26.9\n    }\n}";
    JSON json;
    json << 
        "a" >> "a" <<
        "b" >> 
            json <<
                "aa" >> "aa" <<
                "bb" >> 6 <<
                "cc" >> 2.2
            <<json <<
        "c" >> 
            json <<
                "ab" >> "ab" <<
                "bd" >> false <<
                "gc" >> 26.9
            <<json
    << json;
    String result = json.toString();
    if(result == expect){
        cout<< "test 4 [nested json objs] : passed \n" << expect <<endl;
    } else {
        cout<< "test 4 [nested json objs] : faild"<<endl;
        cout << ":" << result << ":"<< result.length() << endl;
        cout << ":" << expect << ":" << expect.length() << endl;
    }
}

// test 5 (double nested json objs)
void test5(){
    String expect = "{\n    \"a\": \"a\",\n    \"b\": {\n        \"aa\": \"aa\",\n        \"bb\": 6,\n        \"cc\": {\n            \"ab\": \"ab\",\n            \"bd\": 0,\n            \"gc\": 26.9\n        }\n    }\n}";
    JSON json;
    json << 
        "a" >> "a" <<
        "b" >> 
            json <<
                "aa" >> "aa" <<
                "bb" >> 6 <<
                "cc" >> 
                    json <<
                        "ab" >> "ab" <<
                        "bd" >> false <<
                        "gc" >> 26.9
                    << json
            << json
    << json;
    String result = json.toString();
    if(result == expect){
        cout<< "test 5 [doule nested json objs] : passed \n" << expect <<endl;
    } else {
        cout<< "test 5 [double nested json objs] : faild"<<endl;
        cout << ":" << result << ":"<< result.length() << endl;
        cout << ":" << expect << ":" << expect.length() << endl;
    }
}

// test 6 (a normal array single valued)
void test6(){
    String expect = "[\n    \"a\"\n]";
    JSON json;
    json.ARRAY <<
        "a"
    << json.ARRAY;
    String result = json.toString();
    if(result == expect){
        cout<< "test 6 [simeple array] : passed \n" << expect <<endl;
    } else {
        cout << ":" << result << ":"<< result.length() << endl;
        cout << ":" << expect << ":" << expect.length() << endl;
    }
}

// test 7 (a normal array multi valued)
void test7(){
    String expect = "[\n    \"a\",\n    \"b\",\n    \"c\"\n]";
    JSON json;
    json.ARRAY << 
        "a" >>
        "b" >>
        "c"
    << json.ARRAY;
    String result = json.toString();
    if(result == expect){
        cout<< "test 7 [simeple array] : passed \n" << expect <<endl;
    } else {
        cout<< "test 7 [simeple array] : faild"<<endl;
        cout << ":" << result << ":"<< result.length() << endl;
        cout << ":" << expect << ":" << expect.length() << endl;
    }
}

// test 8 (nested json 2d array)
void test8(){
    String expect = "[\n    \"a\",\n    \"b\",\n    \"c\",\n    [\n        \"aa\"\n    ],\n    [\n        \"aa\"\n    ]\n]";
    JSON json;
    json.ARRAY << 
        "a" >>
        "b" >>
        "c" >>
        json.ARRAY <<
            "aa"
        << json.ARRAY >>
        json.ARRAY <<
            "aa"
        << json.ARRAY
    << json.ARRAY;
    String result = json.toString();
    if(result == expect){
        cout<< "test 8 [2d array] : passed \n" << expect <<endl;
    } else {
        cout<< "test 8 [2d array] : faild"<<endl;
        cout << ":" << result << ":"<< result.length() << endl;
        cout << ":" << expect << ":" << expect.length() << endl;
    }
}

// test 9 (mixed json 1)
void test9(){
    String expect = "[\n    \"a\",\n    \"b\",\n    \"c\",\n    [\n        \"aa\"\n    ],\n    {\n        \"aa\": \"1\"\n    }\n]";
    JSON json;
    json.ARRAY << 
        "a" >>
        "b" >>
        "c" >>
        json.ARRAY <<
            "aa"
        << json.ARRAY >>
        json <<
            "aa" >> "1"
        << json
    << json.ARRAY;
    String result = json.toString();
    if(result == expect){
        cout<< "test 9 [mixed array and obj (1)] : passed \n" << expect <<endl;
    } else {
        cout<< "test 9 [mixed array and obj (1)] : faild"<<endl;
        cout << ":" << result << ":"<< result.length() << endl;
        cout << ":" << expect << ":" << expect.length() << endl;
    }
}

// test 10 (mixed json 2)
void test10(){
    String expect = "[\n    \"a\",\n    \"b\",\n    \"c\",\n    [\n        \"aa\"\n    ],\n    {\n        \"aa\": \"1\",\n        \"ports\": [\n            \"a\",\n            \"b\"\n        ]\n    },\n    [\n        \"aa\"\n    ]\n]";
    JSON json;
    json.ARRAY << 
        "a" >>
        "b" >>
        "c" >>
        json.ARRAY <<
            "aa"
        << json.ARRAY >>
        json <<
            "aa" >> "1" <<
            "ports" >>
            json.ARRAY <<
                "a" >>
                "b"
            << json.ARRAY
        << json >>
        json.ARRAY <<
            "aa"
        << json.ARRAY
    << json.ARRAY;
    String result = json.toString();
    if(result == expect){
        cout<< "test 10 [mixed array and obj (2)] : passed \n" << expect <<endl;
    } else {
        cout<< "test 10 [mixed array and obj (2)] : faild"<<endl;
        cout << ":" << result << ":"<< result.length() << endl;
        // cout << ":" << expect << ":" << expect.length() << endl;
    }
}

void tests()
{
    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
    test7();
    test8();
    test9();
    test10();
}
