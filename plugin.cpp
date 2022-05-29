#include <objc/objc.h>
#include <objc/objc-runtime.h>
#include <dlfcn.h>

#include <iostream>

id nsstringWithUTF8(const char *string)
{ // converts CString to NSString
    id NSString = (id)objc_getClass("NSString");
    SEL stringInit = sel_registerName("stringWithUTF8String:");
    return ((id(*)(id, SEL, const char*))objc_msgSend)(NSString, stringInit, string);
}

id urlWithNSString(id NSString)
{
    id NSURL = (id)objc_getClass("NSURL");
    NSURL = ((id(*)(id, SEL))objc_msgSend)(NSURL, sel_registerName("alloc"));
    NSURL = ((id(*)(id, SEL, id))objc_msgSend)(NSURL, sel_registerName("initWithString:"), NSString);
    return NSURL;
}

id urlWithCString(const char *string)
{
    return urlWithNSString(nsstringWithUTF8(string));
}

std::string description(id object)
{
    SEL descriptionSEL = sel_registerName("description");
    id descriptionNSString = ((id(*)(id, SEL))objc_msgSend)(object, descriptionSEL);
    
    SEL cDescriptionSEL = sel_registerName("UTF8String");
    const char* cDescription = ((const char* (*)(id, SEL))objc_msgSend)(descriptionNSString, cDescriptionSEL);
    
    std::string description(cDescription);
    return description;
}

id valueForKey(id dictionary, const char *key)
{ // NSString value for key
    SEL valueSEL = sel_registerName("valueForKey:");
    return ((id(*)(id, SEL, id, SEL))objc_msgSend)(dictionary, valueSEL, nsstringWithUTF8(key), sel_registerName("description"));
}

id paramForObject(id object, const char *param)
{
    return ((id(*)(id, SEL))objc_msgSend)(object, sel_registerName(param));
}

void setValueForKey(id dictionary, id value, id key)
{
    ((id(*)(id, SEL, id, id))objc_msgSend)(dictionary, sel_registerName("setValue:forKey:"), value, key);
}

void setObjectForKey(id dictionary, id object, id key)
{
    ((id(*)(id, SEL, id, id))objc_msgSend)(dictionary, sel_registerName("setObject:forKey:"), object, key);
}

void addEntriesDict(id dictionary, id secondDictionary)
{
    ((id(*)(id, SEL, id))objc_msgSend)(dictionary, sel_registerName("addEntriesFromDictionary:"), secondDictionary);
}

id makeMutableDict()
{
    return ((id(*)(id, SEL))objc_msgSend)((id)objc_getClass("NSMutableDictionary"), sel_registerName("dictionary"));
}

id mutableCopyDict(id dictionary)
{ // creates mutable dictionary
    return ((id(*)(id, SEL, id))objc_msgSend)((id)objc_getClass("NSMutableDictionary"), sel_registerName("dictionaryWithDictionary:"), dictionary);
}

std::string dictToJSON(id dictionary)
{
    id jsonData = ((id(*)(id, SEL, id, int, id))objc_msgSend)((id)objc_getClass("NSJSONSerialization"), sel_registerName("dataWithJSONObject:options:error:"), dictionary, 0, NULL);
    return ((const char* (*)(id, SEL, SEL, id, int, SEL))objc_msgSend)((id)objc_getClass("NSString"), sel_registerName("alloc"), sel_registerName("initWithData:encoding:"), jsonData, 4, sel_registerName("UTF8String"));
}

id nsstringWithNSData(id NSData)
{
  id NSString = (id)objc_getClass("NSString");
  NSString = ((id(*)(id, SEL))objc_msgSend)(NSString, sel_registerName("alloc"));
  SEL stringInit = sel_registerName("initWithData:encoding:");
  return ((id(*)(id, SEL, id, int))objc_msgSend)(NSString, stringInit, NSData, 4);
}

std::string _getAnisetteData()
{
    void * pool = ((id(*)(id, SEL))objc_msgSend)((id)objc_lookUpClass("NSAutoreleasePool"), sel_getUid("alloc"));
    ((id(*)(void*, SEL))objc_msgSend)(pool, sel_getUid("init"));
    
    dlopen("/System/Library/PrivateFrameworks/AuthKit.framework/AuthKit", RTLD_NOW);
    id session = (id)objc_getClass("AKAppleIDSession");
    session = ((id(*)(id, SEL))objc_msgSend)(session, sel_registerName("alloc"));
    session = ((id(*)(id, SEL, id))objc_msgSend)(session, sel_registerName("initWithIdentifier:"), nsstringWithUTF8("com.apple.gs.xcode.auth"));
    id headers = ((id(*)(id, SEL, id))objc_msgSend)(session, sel_registerName("appleIDHeadersForRequest:"), nil);
    id device = ((id(*)(id, SEL))objc_msgSend)((id)objc_getClass("AKDevice"), sel_registerName("currentDevice"));
    
    headers = mutableCopyDict(headers);
    setValueForKey(headers, nsstringWithUTF8("<MacBookPro15,1> <Mac OS X;10.15.2;19C57> <com.apple.AuthKit/1 (com.apple.dt.Xcode/3594.4.19)>"), nsstringWithUTF8("X-MMe-Client-Info"));
    
    setObjectForKey(headers, paramForObject(device, "uniqueDeviceIdentifier"), nsstringWithUTF8("X-Mme-Device-Id"));
    id jsonData = ((id(*)(id, SEL, id, int, id))objc_msgSend)((id)objc_lookUpClass("NSJSONSerialization"), sel_registerName("dataWithJSONObject:options:error:"), headers, 0, NULL);
    std::string anisette = description(nsstringWithNSData(jsonData));
    std::cout << "response: " << anisette.c_str() << std::endl;
    
    ((id(*)(void*, SEL))objc_msgSend)(pool, sel_getUid("release"));
    return anisette;
}
