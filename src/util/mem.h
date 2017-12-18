//
// Created by zts on 17-9-30.
//

#ifndef R2M_PROXY2_MEM_H
#define R2M_PROXY2_MEM_H


#include <cstring>
#include <string>


inline static
std::string str_escape(const char *s, size_t size){
    static const char *hex = "0123456789abcdef";
    std::string ret;
    for(size_t i=0; i<size; i++){
        char c = s[i];
        switch(c){
            case '\r':
                ret.append("\\r");
                break;
            case '\n':
                ret.append("\\n");
                break;
            case '\t':
                ret.append("\\t");
                break;
            case '\\':
                ret.append("\\\\");
                break;
            case ' ':
                ret.push_back(c);
                break;
            default:
                if(c >= '!' && c <= '~'){
                    ret.push_back(c);
                }else{
                    ret.append("\\x");
                    unsigned char d = (unsigned char) c;
                    ret.push_back(hex[d >> 4]);
                    ret.push_back(hex[d & 0x0f]);
                }
                break;
        }
    }
    return ret;
}

inline static
std::string hexmem(const void *p, size_t size){
    return str_escape((char *)p, size);
}

template <typename T>
inline static
std::string hexstr(const T &t){
    return hexmem(t.data(), t.size());
}

#define hexcstr(t) hexstr(t).c_str()

#endif //R2M_PROXY2_MEM_H
