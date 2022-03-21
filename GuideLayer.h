template <typename T>
class GuideLayer : public MetalBaseLayer<T> {
    
    private:
        
        id<MTLTexture> _texture;
        id<MTLBuffer> _texcoordBuffer;
        
    public:
        
        id<MTLTexture> texture() {
            return this->_texture;
        }
    
        T *data() {
            return this->_data;
        }
    
        bool setup() {
            
            MTLTextureDescriptor *textureDescriptor = MTLUtils::descriptor(MTLPixelFormatRGBA8Unorm,this->_width,this->_height);
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
        
        GuideLayer() : MetalBaseLayer<T>() {
            this->_useArgumentEncoder = true;
        }
        
        ~GuideLayer() {}
};
