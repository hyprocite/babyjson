#include<regex>
#include<variant>
#include<unordered_map>
#include<cstring>
#include<vector>
#include<string_view>
#include<string>
#include <memory> 
#include <charconv>
#include "print.h"
struct JsonObject;
using JSONDict = std:: unordered_map<std::string, JsonObject>;
using JSONList = std:: vector<JsonObject>;

struct JsonObject
{
    std::variant
    <
    std::nullptr_t,
    bool,
    int,
    double,
    std::string,
    JSONDict,
    JSONList
    >inner;
     
    void do_print() const
    {
        print(inner);
    }
};

template <class T>
std::optional<T> try_parse_num(std::string_view str)  //将字符类型转换为数字类型
{
    T value;
    auto res = std::from_chars(str.data(), str.data() + str.size(), value);
    if(res.ec == std::errc() && res.ptr == str.data() + str.size())   //后面判断尾地址指针是否正确,检查其是否解析了完整长度的字符串
    {
        return value;
    }
    return std::nullopt;
}

char unescaped_char(char c)
{
    switch(c)
    {
        case 'n' : return '\n';
        case 'r' : return '\r';
        case 't' : return '\t';
        case 'v' : return '\v';
        case '0' : return '\0';
        case 'b' : return '\b';
        case 'f' : return '\f';
        case 'a' : return '\a';
        case '"' : return '\"';
        default :return c;
    }
}

std::pair<JsonObject,int> parse(std::string_view json)
{
    if(json.empty())return {JsonObject{},0};
    else if (size_t off = json.find_first_not_of(" \n\r\t\v\f\0"); off != 0 && off != json.npos) {
        auto [obj, eaten] = parse(json.substr(off));
        return {std::move(obj), eaten + off};
    }
    else if((json[0] >= '0' && json[0]<= '9') || (json[0] == '+' && json[0] == '-'))
    {
        std::regex num_re{"[+-]?[0-9]+(\\.[0-9]*)?([eE][+-]?[0-9]+)?"};
        std::cmatch match;
        if(std::regex_search(json.data(),json.data()+json.size(),match,num_re))
        {
            std::string str = match.str();
            if( auto num = try_parse_num<int>(str))
            {   
                return {JsonObject{num.value()},str.size()};
            }

            if( auto num = try_parse_num<double>(str))
            {
                return {JsonObject{num.value()},str.size()};
            } 
        }
    }
    else if(json[0] == '"')
    {
        std:: string str;
        enum
        {
            RAW,
            ESCAPED,
        }phase = RAW;
        size_t i;
        for( i = 1 ; i < json.size() ; i++)
        {   
            char ch = json[i];
            if(phase == RAW)
            {
                if(ch == '"')
                {   
                    i += 1;
                    break;
                }
                else if( ch == '\\')
                {
                    phase = ESCAPED;
                }
                else{
                    str += ch;
                }
            }
            else
            {
                //跳转
                str += unescaped_char(ch);
                phase = RAW;
            }
        }
        //size_t nextcomma = json.find('"',1); //从一号位开始使用find找到下一" 的位置
       // std::string str{json.substr(1,nextcomma-1)};
        return {JsonObject{std::move(str)},i};
    }
    else if(json[0] == '[')
    {
        std::vector<JsonObject> res;
        size_t i ;
        for(i = 1 ; i < json.size();)
        {
            while(i < json.size() && std::isspace(static_cast<unsigned char>(json[i]))) ++i;
            if(json[i] == ']') 
            {
                i+=1;
                break;
            }
            auto [obj, eaten] = parse(json.substr(i));
            if(eaten == 0)
            {
                i = 0;
                break;
            }
            res.push_back(std::move(obj));
            i += eaten;
            while(i < json.size() && std::isspace(static_cast<unsigned char>(json[i]))) ++i; //空白检查,吃掉所有空格
            if(json[i] == ',')
            {
                i+=1;
            }
        }
        return {JsonObject{std::move(res)},i}; 
    }
    else if(json[0] == '{')
    {
        std::unordered_map<std::string,JsonObject> res;
        size_t i = 1;
        while(i<json.size())
        {   
            while(i < json.size() && std::isspace(static_cast<unsigned char>(json[i]))) ++i;
            if(json[i] == '}')
            {
                i++;
                break;
            }
            auto [keyobj,keyeaten] = parse(json.substr(i));
            if(keyeaten ==0 )
            {
                i = 0;
                break;
            }
            i+=keyeaten;
            if (!std::holds_alternative<std::string>(keyobj.inner)) {
                i = 0;
                break;
            }
            while(i < json.size() && std::isspace(static_cast<unsigned char>(json[i]))) ++i;
            if(json[i] == ':')
            {
                i++;
            }
            std::string key = std::move(std::get<std::string>(keyobj.inner));
            auto [valobj, valeaten] = parse(json.substr(i));
            if(valeaten == 0)

            {
                i = 0;
                break;
            }
            i+= valeaten;
            while(i < json.size() && std::isspace(static_cast<unsigned char>(json[i]))) ++i;
            res.try_emplace(std::move(key), std::move(valobj));
            if (json[i] == ',') {
                i += 1;
            }
            
        }
        return {JsonObject{std::move(res)},i};
    }
    return {JsonObject{std:: nullptr_t{}},0};
}


int main()
{
    std::string_view str  = R"JSON(   {"aaa":
    {"bbb":1},
    "ccc":2})JSON";
   auto [obj ,j] = parse(str);
//    auto& dict = std::get<JSONDict>(obj.inner);
// std::cout << std::get<int>(dict["ccc"].inner) << "\n";   // 输出 2
// auto& inner = std::get<JSONDict>(dict["aaa"].inner);
// std::cout << std::get<int>(inner["bbb"].inner) << "\n";  // 输出 1 
   printnl(obj);
    return 0;
}