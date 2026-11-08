#import <UIKit/UIKit.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <errno.h>
#import <stdint.h>
#import <unistd.h>

// Test harness only: the complete runtime defines ppu_vm_size in its own host initialization.
uint32_t ppu_vm_size = 0;
extern uint8_t *vm_base;
extern int ios_guest_vm_init(void);
extern int ios_guest_vm_shutdown(void);
extern int32_t ios_hle_getpid(void);
extern int ios_hle_check_big_endian(void);
extern void sys_lwmutex_reset_all(void);
extern int ios_lifted_branch(uint64_t input, uint64_t *out, int *table_ok);
extern int ios_lifted_memory(uint32_t input, uint64_t *out, uint32_t *stored, int *bytes_ok);

@interface ProbeApp : UIResponder <UIApplicationDelegate, MTKViewDelegate>
@property (nonatomic, strong) UIWindow *window;
@property (nonatomic, strong) UILabel *label;
@property (nonatomic, strong) MTKView *metalView;
@property (nonatomic, strong) id<MTLCommandQueue> queue;
@property (nonatomic, strong) NSMutableString *report;
@property (nonatomic, assign) NSInteger committed;
@property (nonatomic, assign) NSInteger completed;
@property (nonatomic, assign) BOOL finished;
@property (nonatomic, assign) BOOL pagesOK;
@property (nonatomic, assign) BOOL initOK;
@property (nonatomic, assign) BOOL secondOK;
@property (nonatomic, assign) BOOL hleOK;
@property (nonatomic, assign) BOOL endianOK;
@property (nonatomic, assign) BOOL liftedOK;
@property (nonatomic, assign) BOOL registryOK;
@property (nonatomic, assign) BOOL memoryLiftOK;
@end

@implementation ProbeApp
- (void)note:(NSString *)line {
    NSAssert([NSThread isMainThread], @"Report writes must be on main thread");
    NSLog(@"[PHASE67] %@", line);
    [self.report appendFormat:@"%@\n", line];
    NSURL *dir = [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] firstObject];
    [[self.report copy] writeToURL:[dir URLByAppendingPathComponent:@"phase67-report.txt"] atomically:YES encoding:NSUTF8StringEncoding error:nil];
    self.label.text = self.report;
}
- (void)fail:(NSString *)reason {
    if (self.finished) return;
    self.finished = YES;
    self.metalView.paused = YES;
    [self note:[@"PHASE67_FAIL " stringByAppendingString:reason]];
}
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)options {
    self.report = [NSMutableString new];
    self.window = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
    UIViewController *vc = [UIViewController new];
    vc.view.backgroundColor = [UIColor colorWithRed:.04 green:.09 blue:.16 alpha:1];
    self.window.rootViewController = vc;

    self.label = [UILabel new];
    self.label.translatesAutoresizingMaskIntoConstraints = NO;
    self.label.numberOfLines = 0;
    self.label.textColor = UIColor.whiteColor;
    self.label.font = [UIFont monospacedSystemFontOfSize:10 weight:UIFontWeightRegular];
    self.label.adjustsFontSizeToFitWidth = YES;
    self.label.minimumScaleFactor = 0.65;
    [vc.view addSubview:self.label];
    [NSLayoutConstraint activateConstraints:@[
      [self.label.topAnchor constraintEqualToAnchor:vc.view.safeAreaLayoutGuide.topAnchor constant:10],
      [self.label.leadingAnchor constraintEqualToAnchor:vc.view.leadingAnchor constant:14],
      [self.label.trailingAnchor constraintEqualToAnchor:vc.view.trailingAnchor constant:-14]
    ]];
    [self.window makeKeyAndVisible];
    [self note:@"PHASE67_BEGIN UIKit + PPU lifter + HLE + VM + MetalKit"];
    [self note:[NSString stringWithFormat:@"PAGE_SIZE=%ld",sysconf(_SC_PAGESIZE)]];

    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) { [self fail:@"NO_METAL_DEVICE"]; return YES; }
    self.queue = [device newCommandQueue];
    if (!self.queue) { [self fail:@"NO_COMMAND_QUEUE"]; return YES; }
    [self note:@"METAL_DEVICE_OK"];

    self.metalView = [[MTKView alloc] initWithFrame:CGRectZero device:device];
    self.metalView.translatesAutoresizingMaskIntoConstraints = NO;
    self.metalView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    self.metalView.clearColor = MTLClearColorMake(0.02, 0.48, 0.54, 1.0);
    self.metalView.preferredFramesPerSecond = 30;
    self.metalView.enableSetNeedsDisplay = NO;
    self.metalView.paused = YES;
    self.metalView.delegate = self;
    [vc.view insertSubview:self.metalView belowSubview:self.label];
    [NSLayoutConstraint activateConstraints:@[
      [self.metalView.topAnchor constraintEqualToAnchor:vc.view.topAnchor],
      [self.metalView.leadingAnchor constraintEqualToAnchor:vc.view.leadingAnchor],
      [self.metalView.trailingAnchor constraintEqualToAnchor:vc.view.trailingAnchor],
      [self.metalView.bottomAnchor constraintEqualToAnchor:vc.view.bottomAnchor]
    ]];
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        int rc=ios_guest_vm_init();
        BOOL ok=(rc==0 && vm_base!=NULL && ppu_vm_size==0xE0000000u);
        BOOL pages=ok;
        int32_t hle_pid=-1;
        BOOL endianOK=NO;
        BOOL liftOK=NO, registryOK=NO, memoryLiftOK=NO;
        uint64_t branch0=UINT64_MAX, branch35=0, storedOutput=0;
        uint32_t stored=0;
        int bytesOK=0;
        if (ok) {
            sys_lwmutex_reset_all();
            hle_pid=ios_hle_getpid();
            endianOK=(ios_hle_check_big_endian()==1);
            int tableA=0,tableB=0;
            int r1=ios_lifted_branch(0,&branch0,&tableA);
            int r2=ios_lifted_branch(35,&branch35,&tableB);
            registryOK=(tableA&&tableB);
            liftOK=(r1==0&&r2==0&&branch0==0&&branch35==42&&registryOK);
            int r3=ios_lifted_memory(0x12345678u,&storedOutput,&stored,&bytesOK);
            memoryLiftOK=(r3==0 && storedOutput==0x1234567AULL && stored==0x12345678u && bytesOK==1);
        }
        if (ok) {
            size_t page=(size_t)sysconf(_SC_PAGESIZE);
            size_t offsets[3]={0, 0x40000000u, 0xE0000000u-page};
            for(int i=0;i<3;i++) {
                volatile uint8_t *p=vm_base+offsets[i];
                uint8_t x=(uint8_t)(0x52+i);
                p[0]=x;
                if(p[0]!=x) pages=NO;
            }
        }
        int second=ok?ios_guest_vm_init():-1;
        dispatch_async(dispatch_get_main_queue(), ^{
            self.initOK=ok;
            self.pagesOK=pages;
            self.secondOK=(second==EALREADY);
            self.hleOK=(hle_pid==1001);
            self.endianOK=endianOK;
            self.liftedOK=liftOK;
            self.registryOK=registryOK;
            self.memoryLiftOK=memoryLiftOK;
            [self note:[NSString stringWithFormat:@"PPC_BRANCH_ZERO_OUTPUT=%llu EXPECTED=0",(unsigned long long)branch0]];
            [self note:[NSString stringWithFormat:@"PPC_BRANCH_NONZERO_OUTPUT=%llu EXPECTED=42",(unsigned long long)branch35]];
            [self note:[NSString stringWithFormat:@"PPC_MEMORY_OUTPUT=0x%llx EXPECTED=0x1234567a",(unsigned long long)storedOutput]];
            [self note:[NSString stringWithFormat:@"PPC_MEMORY_STORED=0x%08x BYTES_BIG_ENDIAN=%@",stored,bytesOK?@"YES":@"NO"]];
            [self note:[NSString stringWithFormat:@"PPC_LIFT_FUNCTION_TABLE=%@",registryOK?@"OK":@"FAIL"]];
            [self note:[NSString stringWithFormat:@"PRODUCTION_HLE_GETPID=%d EXPECTED=1001",(int)hle_pid]];
            [self note:[NSString stringWithFormat:@"GUEST_ENDIAN_READ32=%@",endianOK?@"OK":@"FAIL"]];
            [self note:[NSString stringWithFormat:@"VM_INIT_RESULT=%d VM_BASE_NONNULL=%d VM_SIZE=0x%x",rc,vm_base!=NULL,ppu_vm_size]];
            [self note:[NSString stringWithFormat:@"VM_THREE_PAGES=%@ SECOND_INIT_EALREADY=%@",pages?@"OK":@"FAIL",self.secondOK?@"YES":@"NO"]];
            if (!ok || !pages || !self.secondOK || !self.hleOK || !self.endianOK || !self.liftedOK || !self.registryOK || !self.memoryLiftOK) { [self fail:@"VM_OR_HLE_CHECK"]; return; }
            [self note:@"VM_ACTIVE_RENDER_BEGIN"];
            self.metalView.paused = NO;
        });
    });
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(25*NSEC_PER_SEC)),dispatch_get_main_queue(), ^{
        if(!self.finished) [self fail:@"TIMEOUT_WAITING_FOR_METAL_FRAMES"];
    });
    return YES;
}
- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {}
- (void)drawInMTKView:(MTKView *)view {
    if(self.finished || !self.initOK || vm_base==NULL) return;
    if(self.committed>=45) return;
    id<CAMetalDrawable> drawable=view.currentDrawable;
    MTLRenderPassDescriptor *rp=view.currentRenderPassDescriptor;
    if (!drawable || !rp) return;
    id<MTLCommandBuffer> cb=[self.queue commandBuffer];
    if(!cb) { [self fail:@"NO_COMMAND_BUFFER"]; return; }
    id<MTLRenderCommandEncoder> encoder=[cb renderCommandEncoderWithDescriptor:rp];
    if(!encoder) { [self fail:@"NO_RENDER_ENCODER"]; return; }
    [encoder endEncoding];
    [cb presentDrawable:drawable];
    self.committed++;
    __weak ProbeApp *weakSelf=self;
    [cb addCompletedHandler:^(id<MTLCommandBuffer> done) {
        BOOL ok=done.status==MTLCommandBufferStatusCompleted;
        dispatch_async(dispatch_get_main_queue(), ^{
            ProbeApp *strongSelf=weakSelf;
            if(!strongSelf || strongSelf.finished) return;
            if(!ok) { [strongSelf fail:@"METAL_COMMAND_BUFFER_ERROR"]; return; }
            strongSelf.completed++;
            if(strongSelf.completed>=30) [strongSelf completeProbe];
        });
    }];
    [cb commit];
}
- (void)completeProbe {
    if(self.finished) return;
    self.finished=YES;
    self.metalView.paused=YES;
    [self note:[NSString stringWithFormat:@"METAL_FRAMES_COMMITTED=%ld COMPLETED=%ld",(long)self.committed,(long)self.completed]];
    [self note:[NSString stringWithFormat:@"VM_ACTIVE_DURING_METAL=%@",vm_base!=NULL?@"YES":@"NO"]];
    // Stop drawing; the last 30 command buffers are completed before releasing VM.
    int rc=ios_guest_vm_shutdown();
    BOOL clean=(rc==0 && vm_base==NULL && ppu_vm_size==0);
    [self note:[NSString stringWithFormat:@"VM_SHUTDOWN_RESULT=%d GLOBALS_RESET=%@",rc,clean?@"YES":@"NO"]];
    if(clean && self.pagesOK && self.secondOK && self.hleOK && self.endianOK && self.liftedOK && self.registryOK && self.memoryLiftOK && self.completed>=30) [self note:@"PHASE67_PASS"];
    else [self note:@"PHASE67_FAIL final_checks"];
}
@end
int main(int argc,char *argv[]) { @autoreleasepool {
    return UIApplicationMain(argc,argv,nil,NSStringFromClass([ProbeApp class]));
} }
