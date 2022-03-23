template <typename T>
class MeshLayer : public MetalBaseLayer<T> {
    
    private:
    
        unsigned int *MAP = nullptr;
        MTLReadPixels<unsigned short> *ReadPixels = nullptr;
    
        id<MTLTexture> _texture;
        id<MTLBuffer> _texcoordBuffer;

    public:
        
        T *data() {
            return this->_data;
        }
    
        bool setup() {
            
            MTLTextureDescriptor *textureDescriptor = MTLUtils::descriptor(MTLPixelFormatRG16Unorm,this->_width,this->_height);
            if(!textureDescriptor) return false;
            
            this->_texture = [this->_device newTextureWithDescriptor:textureDescriptor];
            if(!this->_texture) return false;
                        
            if(this->DEPTH_TEST) {
                MTLTextureDescriptor *depthTextureDescriptor = MTLUtils::descriptor(MTLPixelFormatDepth32Float_Stencil8,this->_width,this->_height);
                if(!depthTextureDescriptor) return false;
                this->_depthTexture = [this->_device newTextureWithDescriptor:depthTextureDescriptor];
                if(!this->_depthTexture) return false;
                MTLDepthStencilDescriptor *depthDescriptor = [MTLDepthStencilDescriptor new];
                depthDescriptor.depthCompareFunction = MTLCompareFunctionLess;
                depthDescriptor.depthWriteEnabled = YES;
                this->_depthState = [this->_device newDepthStencilStateWithDescriptor:depthDescriptor];
                if(!this->_depthState) return false;
            }
                        
            if(MetalBaseLayer<T>::setup()==false) return false;

            this->_texcoordBuffer = [this->_device newBufferWithBytes:this->_data->texcoord length:this->_data->TEXCOORD_SIZE*sizeof(float) options:MTLResourceOptionCPUCacheModeDefault];
            if(!this->_texcoordBuffer) return false;
            
            this->_argumentEncoderBuffer = [this->_device newBufferWithLength:sizeof(float)*[this->_argumentEncoder encodedLength] options:MTLResourceOptionCPUCacheModeDefault];

            [this->_argumentEncoder setArgumentBuffer:this->_argumentEncoderBuffer offset:0];
            [this->_argumentEncoder setTexture:this->_texture atIndex:0];
            
            return true;
        }
    
        id<MTLCommandBuffer> setupCommandBuffer() {
            
            // randomize
            
            [this->_texture replaceRegion:MTLRegionMake2D(0,0,this->_width,this->_height) mipmapLevel:0 withBytes:this->MAP bytesPerRow:this->_width<<2];
            
            Mesh *mesh = this->_data;
            
            int w = mesh->width();
            int h = mesh->height();
            
            //NSLog(@"%d,%d",w,h);
            
            float *vertices = (float *)[this->_verticesBuffer contents];
            
            for(int i=1; i<h-1; i++) {
                for(int j=1; j<w-1; j++) {
                    
                    int addr = (i*w+j)<<2;

                    vertices[addr+0] = (j/((float)(w-1)))*2.0-1.0 + (((random()%200)*0.01)-1.0)/(float)(w-1);
                    vertices[addr+1] = (i/((float)(h-1)))*2.0-1.0 + (((random()%200)*0.01)-1.0)/(float)(h-1);
                    
                }
            }
            
            id<MTLCommandBuffer> commandBuffer = [this->_commandQueue commandBuffer];
            
            this->setupColorAttachment(this->_renderPassDescriptor.colorAttachments[0]);
            if(this->DEPTH_TEST) {
                this->setupDepthAttachment(this->_renderPassDescriptor.depthAttachment);
            }
            
            id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:this->_renderPassDescriptor];
            
            [renderEncoder setRenderPipelineState:this->_renderPipelineState];
            [renderEncoder setVertexBuffer:this->_verticesBuffer offset:0 atIndex:0];
            [renderEncoder setVertexBuffer:this->_texcoordBuffer offset:0 atIndex:1];
        
            if(this->DEPTH_TEST) {
                [renderEncoder setDepthStencilState:this->_depthState];
            }
            
            [renderEncoder useResource:this->_texture usage:MTLResourceUsageSample];
            
            [renderEncoder setFragmentBuffer:this->_argumentEncoderBuffer offset:0 atIndex:0];

            if(this->_data->INDICES_TYPE==sizeof(unsigned short)) {
                [renderEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:this->_data->INDICES_SIZE indexType:MTLIndexTypeUInt16 indexBuffer:this->_indicesBuffer indexBufferOffset:0];
            }
            else {
                [renderEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:this->_data->INDICES_SIZE indexType:MTLIndexTypeUInt32 indexBuffer:this->_indicesBuffer indexBufferOffset:0];
            }
            
            [renderEncoder endEncoding];
            
            [commandBuffer presentDrawable:this->_metalDrawable];
            this->_drawabletexture = this->_metalDrawable.texture;
            
            return commandBuffer;
        }
    
        bool init(int width, int height, NSString *shader=@"default.metallib", NSString *identifier=nil, bool blendingEnabled=false) {
            
            if(!this->MAP) {
                this->MAP = new unsigned int[width*height];
                for(int i=0; i<height; i++) {
                    for(int j=0; j<width; j++) {
                        this->MAP[i*width+j] = (0x5555+((int)(j*SCALE)))<<16|(0x5555+((int)(i*SCALE)));
                    }
                }
            }
            
            if(!this->ReadPixels) {
               this->ReadPixels = new MTLReadPixels<unsigned short>(width,height,2);
            }
            
            if(!this->_data) {
                this->_data = new T(width>>3,height>>3);
            }
                    
            return MetalBaseLayer<T>::init(width,height,shader,identifier,blendingEnabled);

        }
    
        unsigned int *getByte() {
            
            unsigned int *bytes = (unsigned int *)this->ReadPixels->getBytes(this->drawableTexture(),true);

            return bytes;
        }
       
        MeshLayer() : MetalBaseLayer<T>() {
            this->_useArgumentEncoder = true;
        }
        
        ~MeshLayer() {}
};
