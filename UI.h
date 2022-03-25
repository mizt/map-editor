class Footer {
    
    private:
    
        bool _init = false;
    
        WKWebViewConfiguration *_webConfiguration = [[WKWebViewConfiguration alloc] init];
        WKUserContentController *_userController = [[WKUserContentController alloc] init];
        WKWebView<WKNavigationDelegate,WKScriptMessageHandler> *_view;
      
    public:
    
        WKWebView *view() {
            return this->_view;
        }
    
        void scale(int v) {
            [this->_view evaluateJavaScript:[NSString stringWithFormat:@"document.getElementById('scale').textContent = %d;",v] completionHandler:nil];
        }
    
        Footer() {
            
            if(objc_getClass("Web")==nil) {
                
                objc_registerClassPair(objc_allocateClassPair(objc_getClass("WKWebView"),"Web",0));
                Class Web = objc_getClass("Web");

                Utils::addMethod(Web,@"webView:didFinishNavigation:",^(id me, WKWebView *webView, WKNavigation *navigation) {
                    if(this->_init==false) {
                        NSLog(@"webView:didFinishNavigation:");
                        [this->_view evaluateJavaScript:@"document.body.setAttribute('oncontextmenu','event.preventDefault();');" completionHandler:nil];
                        this->_init = true;
                        [this->_view setAlphaValue:1];
                        
    #ifndef TARGET_OS_OSX
                        
                        this->_view.scrollView.minimumZoomScale = 1.0;
                        this->_view.scrollView.maximumZoomScale = 1.0;
                        
                        for(id subview in [this->_view subviews]) {
                            if([subview isKindOfClass:[UIScrollView class]]) {
                                ((UIScrollView*)subview).bounces = NO;
                            }
                        }

                        this->_view.scrollView.bounces = NO;
                        this->_view.scrollView.canCancelContentTouches = NO;
                        this->_view.scrollView.scrollEnabled = NO;
                        
    #endif
                                    
                    }
                },"v@:@@");
                
                Utils::addMethod(Web,@"userContentController:didReceiveScriptMessage:",^(id me,WKUserContentController *userContentController,WKScriptMessage *message) {
                                    
                    if(Utils::isEqString(message.name,@"select")) {
                        [[NSNotificationCenter defaultCenter] postNotification:[NSNotification notificationWithName:message.body object:nil]];
                    }
                
                },"v@:@@");
            }
            
            this->_webConfiguration.userContentController = this->_userController;

            this->_view = (WKWebView<WKNavigationDelegate,WKScriptMessageHandler> *)[[objc_getClass("Web") alloc] initWithFrame:CGRectMake(0,0,STAGE_WIDTH,FOOTER_HEIGHT) configuration:this->_webConfiguration];
            this->_view.layer.backgroundColor = [NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1].CGColor;
            
            [this->_userController addScriptMessageHandler:this->_view name:@"select"];
            
#ifndef TARGET_OS_OSX
            
            [this->_view setMultipleTouchEnabled:false];
            
#endif
            
            [this->_view setNavigationDelegate:this->_view];
            [this->_view setAlphaValue:0];

            [this->_view loadHTMLString:[NSString stringWithUTF8String:R"(<!DOCTYPE html>
<html>
    <head>
        <meta charset="UTF-8">
        <title></title>
        <style>

            * {
                margin:0;
                padding:0;
                -webkit-user-select:none;
                cursor:default;
                font-size:14px;
                font-family:Helvetica,sans-serif;
                letter-spacing:0.025em;
                color:#CCC;
            }
            
            #footer {
                width:100%;
                height:32px;
                background:rgb(64,64,64);
            }
            
            label {
                padding:0 0 0 12px;
                margin-right:4px;
                font-size:14px;
                line-height:32px;
                display:inline-block;
                cursor:pointer;
                position:relative;
            }
            
            input[type="radio"] {
                display:none;
            }
            
            input[type="radio"]:checked + label:after {
                content:'';
                position:absolute;
                width:8px;
                height:8px;
                top:12px;
                left:0px;
                background-color:#CCC;
                border-radius:50%;
            }
            
            label:before {
                content:'';
                position:absolute;
                width:4px;
                height:4px;
                top:14px;
                left:2px;
                background-color:#CCC;
                border-radius:50%;
            }
            
            #footer > p {
                position:absolute;
                top:0;
                left:calc(100% - 58px);
                text-align:right;
                display:inline-block;
                width:50px;
                line-height:32px;
                -webkit-user-select:none;
                pointer-events:none;
            }
            
        </style>
    </head>
    <body>
        <div id="footer">
            <form style="margin-left:9px;">
                <input id="RGB" type="radio" checked name="mode" />
                <label for="RGB">RGB</label>
                <input id="MAP" type="radio" name="mode" />
                <label for="MAP">Map</label>
            </form>
            <p><span id="scale">100</span><span style="margin-left:0.075em;">%</span></p>
        </div>
        <script>
            document.getElementById("RGB").onchange=(e)=>{
                window.webkit.messageHandlers.select.postMessage("RGB");
            };
            document.getElementById("MAP").onchange=(e)=>{
                window.webkit.messageHandlers.select.postMessage("MAP");

            };
        </script>
    </body>
</html>)"] baseURL:nil];
        }
    
        ~Footer() {
            [this->_view removeFromSuperview];
        }
    
};
