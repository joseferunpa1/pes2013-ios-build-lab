// Phase 76: UIKit host for the real ps3recomp PPU loader/indirect dispatcher.
// The PowerPC instructions are synthetic. Production loader and providers are original.
#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import <dispatch/dispatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Compiled from .ci/phase72/production_dispatch_probe.cpp with main renamed.
int phase76_probe_main(void);

@interface ProductionAppDelegate : UIResponder <UIApplicationDelegate>
@property (strong, nonatomic) UIWindow *window;
@end

@implementation ProductionAppDelegate
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)options {
    (void)application;
    (void)options;
    self.window = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
    UIViewController *controller = [UIViewController new];
    controller.view.backgroundColor = [UIColor colorWithRed:0.03 green:0.13 blue:0.19 alpha:1.0];
    UILabel *label = [[UILabel alloc] initWithFrame:CGRectInset(controller.view.bounds, 20, 80)];
    label.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    label.textColor = [UIColor whiteColor];
    label.numberOfLines = 0;
    label.textAlignment = NSTextAlignmentCenter;
    label.text = @"PS3 recompilation — Phase 76\nGuest bctrl -> production PPU dispatcher\nSee GitHub Actions report";
    [controller.view addSubview:label];
    self.window.rootViewController = controller;
    [self.window makeKeyAndVisible];
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        @autoreleasepool {
            NSString *documents = [NSHomeDirectory() stringByAppendingPathComponent:@"Documents"];
            [[NSFileManager defaultManager] createDirectoryAtPath:documents
                                       withIntermediateDirectories:YES attributes:nil error:nil];
            NSString *path = [documents stringByAppendingPathComponent:@"phase76-report.txt"];
            FILE *stream = freopen(path.fileSystemRepresentation, "w", stdout);
            if (!stream) return;
            // Merge stderr for diagnostics and flush after each line.
            dup2(fileno(stdout), fileno(stderr));
            setvbuf(stdout, NULL, _IONBF, 0);
            puts("PHASE76_APP_STARTED");
            fflush(stdout);
            int result = phase76_probe_main();
            printf("PHASE76_PROBE_RETURN=%d\n", result);
            puts(result == 0 ? "PHASE76_RUNTIME_PASS" : "PHASE76_RUNTIME_FAIL");
            fflush(stdout);
        }
    });
    return YES;
}
@end

int main(int argc, char **argv) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([ProductionAppDelegate class]));
    }
}
