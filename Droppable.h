class Droppable {
    
    protected:
    
        bool _isDrop = true;
        bool _isDrag = false;
        
        float _wheel = 0.5;
    
        Content *_content = nullptr;
        Footer *_footer = new Footer();
    
    public:
    
        Droppable() {
            
            if(objc_getClass("DroppableView")==nil) { objc_registerClassPair(objc_allocateClassPair(objc_getClass("NSView"),"DroppableView",0)); }
            
            Class DroppableView = objc_getClass("DroppableView");

            if(DroppableView) {
                Utils::addMethod(DroppableView,@"otherMouseDragged:",^(id me,NSEvent *theEvent) {
                    if(this->_isDrag==false&&this->_content) {
                        if(theEvent.buttonNumber==2) {
                            this->_content->translate(
                            this->_content->tx()+theEvent.deltaX,
                            this->_content->ty()-theEvent.deltaY);
                            this->_content->transform();
                        }
                    }
                },"@@:@");
                
                Utils::addMethod(DroppableView,@"scrollWheel:",^(id me,NSEvent *theEvent) {
                    if(this->_isDrag==false&&this->_content) {
                        float divsion = 4.0;
                        this->_wheel+=(theEvent.deltaY/100.0)*8.0;
                        this->_wheel = (this->_wheel<=0.0)?0.0:(this->_wheel>=1.0)?1.0:this->_wheel;
                        if(this->_wheel>0.5) {
                            this->_content->scale(((int)((1.0+(this->_wheel-0.5)*2.0)*divsion))*(1.0/divsion));
                        }
                        else {
                            this->_content->scale(((int)((0.25+(0.75*2.0)*this->_wheel)*divsion))*(1.0/divsion));
                        }
                        this->_content->transform();
                        if(this->_footer) this->_footer->scale((int)(this->_content->scale()*100.0));
                    }
                },"@@:@");
                
                Utils::addMethod(DroppableView,@"draggingEntered:",^NSDragOperation(id me,id<NSDraggingInfo> sender) {
                    if(!this->_isDrop) return NSDragOperationNone;
                    return NSDragOperationLink;
                },"@@:@");

                Utils::addMethod(DroppableView,@"performDragOperation:",^BOOL(id me,id<NSDraggingInfo> sender) {
                    return this->_isDrop?YES:NO;
                },"@@:@");
                
                /*
                Utils::addMethod(DroppableView,@"concludeDragOperation:",^(id me,id<NSDraggingInfo> sender) {
                
                },"@@:@");
                */
                
            }
        }
    
        ~Droppable() {
    
        }
    
};