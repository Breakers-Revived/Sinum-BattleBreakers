// Copyright (c) 2024 Project Nova LLC

#import <Foundation/Foundation.h>
#import <objc/runtime.h>

NSString* gApiUrl = @"http://127.0.0.1";

//
//  DYNAMIC PART
//
(BOOL) loadConfigFile() {
    NSString* configPath = [[NSBundle mainBundle] pathForResource:@"sinum_cfg" ofType:@"txt"];
    if (!configPath) {
        NSLog(@"sinum_cfg.txt not found in bundle.");
        return false;
    }
    NSError* error = nil;
    NSString* configContents = [NSString stringWithContentsOfFile:configPath encoding:NSUTF8StringEncoding error:&error];
    if (error) {
        NSLog(@"Failed to load sinum config file: %@", error);
        return false;
    }

    if (!configContents) {
        NSLog(@"Failed to load sinum_cfg.txt. Using default API_URL: %@", gApiUrl);
        return false;
    }

    // Trim and remove newline characters from the configuration content
    NSString* trimmedConfig = [[configContents stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]]
                                stringByReplacingOccurrencesOfString:@"\n" withString:@""];
    trimmedConfig = [trimmedConfig stringByReplacingOccurrencesOfString:@"\r" withString:@""];

    // Set API_URL dynamically
    gApiUrl = trimmedConfig;
    NSLog(@"Loaded and set API_URL from config: %@", gApiUrl);
    return true;
}

#define API_URL gApiUrl

//
//  Actual Sinum hook 
//
@interface CustomURLProtocol : NSURLProtocol
@end

@implementation CustomURLProtocol

+ (BOOL)canInitWithRequest:(NSURLRequest *)request 
{  
    NSString *absoluteURLString = [[request URL] absoluteString];
    if ([NSURLProtocol propertyForKey:@"Handled" inRequest:request]) {
        return NO;
    }
    return YES;
}

+ (NSURLRequest*)canonicalRequestForRequest:(NSURLRequest*)request
{
    return request;
}

- (void)startLoading
{
    NSMutableURLRequest* modifiedRequest = [[self request] mutableCopy];

    NSString* originalPath = [modifiedRequest.URL path];
    NSString* originalQuery = [modifiedRequest.URL query];

    NSString* newBaseURLString = API_URL;
    NSURLComponents* components = [NSURLComponents componentsWithString:newBaseURLString];

    components.path = originalPath;
    if (originalQuery)
    {
        components.query = originalQuery;
    }

    [modifiedRequest setURL:components.URL];
    [NSURLProtocol setProperty:@YES forKey:@"Handled" inRequest:modifiedRequest];

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnonnull"
    [[self client] URLProtocol:self
        wasRedirectedToRequest:modifiedRequest
              redirectResponse:nil];
#pragma clang diagnostic pop
}

- (void)stopLoading
{
}
@end

__attribute__((constructor)) void entry()
{    
    loadConfigFile();
    [NSURLProtocol registerClass:[CustomURLProtocol class]];
}