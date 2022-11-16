#include <CoreFoundation/CoreFoundation.h>
#include <string>
#include <iostream>
//#include "contact.h"

#define USE_ALT
#ifdef USE_ALT
#define USERID "030B0AD6-E1D7-4569-8D8F-A78582461EA7"
#define FETCHER "com.rileytestut.AltServer.FetchAnisetteData"
#define OBSERVER "com.rileytestut.AltServer.AnisetteDataResponse"
#else
#define FETCHER "com.DebianArch.AnisettePlugin.Fetch"
#define OBSERVER "com.DebianArch.AnisettePlugin.Response"
#endif
std::string parseArchivedData(CFDataRef cf_anisette);

std::string globalAnisette = "";
CFNotificationCenterRef center = CFNotificationCenterGetDistributedCenter();
dispatch_semaphore_t sema = dispatch_semaphore_create(0);
void notificationHandler(CFNotificationCenterRef center,
                         void * observer,
                         CFStringRef name,
                         const void * object,
                         CFDictionaryRef userInfo)
{
    printf("Handling response.\n");
#ifdef USE_ALT
    CFDataRef cf_anisette = NULL;
#else
    CFStringRef cf_anisette = NULL;
#endif
    CFDictionaryGetValueIfPresent(userInfo, CFSTR("anisetteData"), (const void**)&cf_anisette);

#ifdef USE_ALT
    CFPropertyListRef plist = CFPropertyListCreateWithData(NULL, cf_anisette, kCFPropertyListImmutable, NULL, NULL);
    globalAnisette = parseArchivedData(cf_anisette).c_str();
#else
    const char *utf8Str = CFStringGetCStringPtr(cf_anisette, kCFStringEncodingMacRoman);
    globalAnisette = utf8Str;
#endif
//    CFRelease(cf_anisette);
    std::cout << "response: " << globalAnisette << std::endl;
    dispatch_semaphore_signal(sema);
};

typedef struct __CFRuntimeBase {
    uintptr_t _cfisa;
    uint8_t _cfinfo[4];
#if __LP64__
    uint32_t _rc;
#endif
} CFRuntimeBase;

struct __CFKeyedArchiverUID {
  CFRuntimeBase _base;
  uint32_t _value;
};

#ifdef USE_ALT
#include <objc/objc.h>
#include <objc/objc-runtime.h>
std::string objc_description(id object)
{
    SEL descriptionSEL = sel_registerName("description");
    id descriptionNSString = ((id(*)(id, SEL))objc_msgSend)(object, descriptionSEL);
    
    SEL cDescriptionSEL = sel_registerName("UTF8String");
    const char* cDescription = ((const char* (*)(id, SEL))objc_msgSend)(descriptionNSString, cDescriptionSEL);
    
    std::string description(cDescription);
    return description;
}

#include <map>
std::map<std::string, std::string> mappings = {
    { "locale", "X-Apple-Locale" },
    { "deviceDescription", "X-MMe-Client-Info" },
    { "localUserID", "X-Apple-I-MD-LU" },
    { "timeZone", "X-Apple-I-TimeZone" },
    { "oneTimePassword", "X-Apple-I-MD" },
    { "machineID", "X-Apple-I-MD-M" },
    { "deviceUniqueIdentifier", "X-Mme-Device-Id" },
    { "date", "X-Apple-I-Client-Time" },
    { "deviceSerialNumber", "X-Apple-I-SRL-NO" },
    { "routingInfo", "X-Apple-I-MD-RINFO" }
};

std::string parseArchivedData(CFDataRef cf_anisette)
{
    /*
        "$objects" => [
            0 => "$null",
            1 => {
                "$class" = "";
                "key1" = "<>{value = 2}";
            }
            2 => "value2"
        ]
     */
    CFDictionaryRef plist = (CFDictionaryRef)CFPropertyListCreateWithData(NULL, cf_anisette, kCFPropertyListImmutable, NULL, NULL);
    CFArrayRef objects = (CFArrayRef)CFDictionaryGetValue(plist, CFSTR("$objects"));
    CFDictionaryRef keyDict = (CFDictionaryRef)CFArrayGetValueAtIndex(objects, 1);
    
    CFIndex keyDictCount = CFDictionaryGetCount(keyDict);
    const void *keys[keyDictCount];
    const void *values[keyDictCount];
    CFDictionaryGetKeysAndValues(keyDict,keys,values);
    std::string ret = "{";
    for (int i = 0; i < (int)keyDictCount; i++)
    {
        id yo = (id)(keys[i]);
        std::string rep = objc_description(yo);
        if (rep.compare("$class") == 0)
            continue;
        
        if (ret.length() > 1)
            ret.append(", ");
        if (rep.compare("locale") == 0)
        {
            ret.append("\"" + mappings.at(rep) + "\":\"en_US\"");
            continue;
        }
        else if (rep.compare("timeZone") == 0)
        {
            ret.append("\"" + mappings.at(rep) + "\":\"PST\"");
            continue;
        }
        else if (rep.compare("date") == 0)
        {
            time_t timer;
            char buffer[26];
            struct tm* tm_info;
            
            timer = time(NULL);
            tm_info = localtime(&timer);
            
            strftime(buffer, 26, "%Y-%m-%dT%H:%M:%SZ", tm_info); // %Y-%m-%dT%H:%M:%SZ a.k.a YYYY-MM-DDThh:mm:ssZ
            ret.append("\"" + mappings.at(rep) + "\":\"" + buffer + "\"");
            continue;
        }
        else if (rep.compare("deviceDescription") == 0)
        {
            ret.append("\"" + mappings.at(rep) + "\":\"<MacBookPro15,1> <Mac OS X;10.15.2;19C57> <com.apple.AuthKit/1 (com.apple.dt.Xcode/3594.4.19)>\"");
            continue;
        }
        
        int index = (*(__CFKeyedArchiverUID*)(values[i]))._value;
        std::string value = objc_description((id)CFArrayGetValueAtIndex(objects, index));
        ret.append("\"" + mappings.at(rep) + "\":\"" + value + "\"");
    }
    ret.append("}");
    CFRelease(plist);
    return ret;
}
#endif

void sendAnisetteRequest()
{
#ifdef USE_ALT
    const void *keys[] = {(void *)CFSTR("requestUUID")};
    const void *values[] = {CFSTR(USERID)};
    CFDictionaryRef userInfo = CFDictionaryCreate(NULL, keys, values, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
#endif
    CFNotificationCenterPostNotificationWithOptions( // this seems to work fine...
                                         center,
                                         CFSTR(FETCHER),
                                         NULL,
#ifndef USE_ALT
                                         NULL,
#else
                                         userInfo,
#endif
                                         kCFNotificationDeliverImmediately
                                         );
#ifdef USE_ALT
    CFRelease(userInfo);
#endif
}

std::string fetchAnisette()
{
    sema = dispatch_semaphore_create(0);
    sendAnisetteRequest();
    if (dispatch_semaphore_wait(sema, dispatch_time(DISPATCH_TIME_NOW, 4 * NSEC_PER_SEC)) != 0)
    {
        printf("Anisette fetch timed out. Make sure AnisettePlugin is enabled and Mail app is open.\n");
    }
    return globalAnisette;
}

#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
void webserver(int port)
{ // for now converting notifications into a webserver for ease...
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Failed creating socket.");
        exit(EXIT_FAILURE);
    }
    
    const int enable = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    memset(address.sin_zero, '\0', sizeof address.sin_zero);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("Failed binding.");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0)
    {
        perror("Failed listening.");
        exit(EXIT_FAILURE);
    }
    
    printf("Ready for incoming connections...\n\n");
    while(1)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0)
        {
            perror("Failed accepting request.");
            continue;
        }
        
        std::string anisette = fetchAnisette();
        if(anisette.length() < 1)
        {
            anisette = "{}";
        }
        
        char msg[anisette.length() + 68];
        sprintf(msg, "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: %lu\n\n%s", anisette.length(), anisette.c_str());
        write(new_socket,msg,strlen(msg));
        close(new_socket);
    }
}

#include <notify.h>
#include <notify_keys.h>

#include <dispatch/dispatch.h>
int main()//Start()
{ // Starts redirecting anisette response to a mini webserver...
    CFNotificationCenterAddObserver
    (
     center,
     NULL,
     notificationHandler,
     CFSTR(OBSERVER),
     NULL,
     CFNotificationSuspensionBehaviorDeliverImmediately
     ); // listen for requests
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
        webserver(6970);
    });
    CFRunLoopRun();
}
