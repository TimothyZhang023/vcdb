#ifndef NET_RESP_H_
#define NET_RESP_H_

#include <unistd.h>
#include <string>
#include <vector>
#include <map>

#define addReplyErrorCodeReturn(n) resp->addReplyError(GetErrorInfo(n)); return 0
#define addReplyErrorInfoReturn(c) resp->addReplyError((c)); return 0


class Response {
private:
    std::string *output = nullptr;

public:
    explicit Response(std::string *output);


    void addReplyError(const std::string &err_msg);

    void addReplyError(const char *err_msg);

    void addReplyNil();

    void addReplyStatus(const std::string &msg);

    void addReplyStatusOK();

    void addReplyString(const std::string &msg);

    void addReplyBulkCBuffer(const void *p, size_t len);

    void addReplyBulkCString(const char *s);

    void addReplyHumanLongDouble(long double d);

    void addReplyDouble(double d);

    void addReplyInt(uint64_t i);

    void addReplyInt(int64_t i);

    void addReplyInt(int i);

    void addReplyListEmpty();

    void addReplyListHead(int size);


    std::vector<std::string> resp_arr;

    void convertReplyToList();

    void convertReplyToScanResult();


};

#endif
