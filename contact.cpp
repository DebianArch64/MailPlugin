//
//  main.cpp
//  AnisetteContactC
//
//  Created by DebianArch on 5/19/22.
//

#include <CoreFoundation/CoreFoundation.h>
#include <string>
#include <iostream>

std::string globalAnisette = "";
CFNotificationCenterRef center = CFNotificationCenterGetDistributedCenter();
dispatch_semaphore_t sema = dispatch_semaphore_create(0);
void notificationHandler(CFNotificationCenterRef center,
                           void * observer,
                           CFStringRef name,
                           const void * object,
                           CFDictionaryRef userInfo)
{
  CFStringRef cf_anisette = NULL;
  CFDictionaryGetValueIfPresent(userInfo, CFSTR("anisetteData"), (const void**)&cf_anisette);
  const char *utf8Str = CFStringGetCStringPtr(cf_anisette, kCFStringEncodingMacRoman);
  globalAnisette = utf8Str;
  std::cout << "response: " << globalAnisette << std::endl;
  dispatch_semaphore_signal(sema);
};

void sendAnisetteRequest()
{
    CFNotificationCenterPostNotification( // this seems to work fine...
                                         center,
                                         CFSTR("com.DebianArch.AnisettePlugin.Fetch"),
                                         NULL,
                                         NULL,
                                         TRUE
                                         );
}

std::string fetchAnisette()
{
    sema = dispatch_semaphore_create(0);
    sendAnisetteRequest();
    if (dispatch_semaphore_wait(sema, dispatch_time(DISPATCH_TIME_NOW, 4 * NSEC_PER_SEC)) != 0)
    {
        printf("Anisette fetch timed out. Make sure AnisettePlugin is installed.\n");
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
void Start()
{ // Starts redirecting anisette response to a mini webserver...
    CFNotificationCenterRef center = CFNotificationCenterGetDistributedCenter();
    CFNotificationCenterAddObserver
    (
     center,
     NULL,
     notificationHandler,
     CFSTR("com.DebianArch.AnisettePlugin.Response"),
     NULL,
     CFNotificationSuspensionBehaviorDeliverImmediately
     ); // listen for requests
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
        webserver(6970);
    });
    CFRunLoopRun();
}

int main()
{
    Start();
    return 0;
}
