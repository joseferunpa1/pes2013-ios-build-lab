// Phase 77 — UIKit host running ORIGINAL production ELF loader in a simulator.
#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import <dispatch/dispatch.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
extern "C" int phase77_probe_main(const char* elf_path);

@interface ProductionAppDelegate : UIResponder <UIApplicationDelegate>
@property (strong, nonatomic) UIWindow *window;
@end
@implementation ProductionAppDelegate
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)options {
    (void)application; (void)options;
    self.window=[[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
    UIViewController *controller=[UIViewController new];
    controller.view.backgroundColor=[UIColor colorWithRed:0.03 green:0.13 blue:0.19 alpha:1.0];
    UILabel *label=[[UILabel alloc] initWithFrame:CGRectInset(controller.view.bounds,20,80)];
    label.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
    label.textColor=UIColor.whiteColor;
    label.numberOfLines=0;
    label.textAlignment=NSTextAlignmentCenter;
    label.text=@"ps3recomp Phase 77\nProduction ELF PT_LOAD + OPD test\nSee GitHub Actions report";
    [controller.view addSubview:label];
    self.window.rootViewController=controller;
    [self.window makeKeyAndVisible];
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED,0), ^{
        @autoreleasepool {
            NSString *docs=[NSHomeDirectory() stringByAppendingPathComponent:@"Documents"];
            [[NSFileManager defaultManager] createDirectoryAtPath:docs withIntermediateDirectories:YES attributes:nil error:nil];
            NSString *report=[docs stringByAppendingPathComponent:@"phase77-report.txt"];
            FILE *stream=freopen(report.fileSystemRepresentation,"w",stdout);
            if(!stream) return;
            dup2(fileno(stdout),fileno(stderr));
            setvbuf(stdout,nullptr,_IONBF,0);
            puts("PHASE77_APP_STARTED");
            NSString *path=[[NSBundle mainBundle] pathForResource:@"tiny-ppu" ofType:@"elf"];
            if(!path) { puts("PHASE77_ELF_BUNDLE_RESOURCE_MISSING"); return; }
            int result=phase77_probe_main(path.fileSystemRepresentation);
            printf("PHASE77_PROBE_RETURN=%d\n",result);
            puts(result==0?"PHASE77_RUNTIME_PASS":"PHASE77_RUNTIME_FAIL");
        }
    });
    return YES;
}
@end
int main(int argc,char**argv) {
    @autoreleasepool { return UIApplicationMain(argc,argv,nil,NSStringFromClass([ProductionAppDelegate class])); }
}
