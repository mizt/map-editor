class Smudge : public MapRG16UnormBase {
    
    public:
    
        void setMap(unsigned int *map) {
            for(int i=0; i<this->height; i++) {
                for(int j=0; j<this->width; j++) {
                    this->_map[0][i*this->width+j] = map[i*this->width+j];
                    this->_map[1][i*this->width+j] = map[i*this->width+j];
                }
            }
        }
    
        void resetMap() {
            for(int i=0; i<this->height; i++) {
                for(int j=0; j<this->width; j++) {
                    this->_map[0][i*this->width+j] = ((int)(0x5555+j*SCALE))<<16|(int)(0x5555+i*SCALE);
                    this->_map[1][i*this->width+j] = ((int)(0x5555+j*SCALE))<<16|(int)(0x5555+i*SCALE);
                }
            }
        }
        
        void reset(float mx,float my) {
            float *prevCursor = (float *)[this->_params[0] contents];
            float *cursor = (float *)[this->_params[1] contents];
            
            prevCursor[0] = cursor[0] = mx;
            prevCursor[1] = cursor[1] = my;
        }
    
        unsigned int *map(float mx,float my) {
            
            float *prevCursor = (float *)[this->_params[0] contents];
            float *cursor = (float *)[this->_params[1] contents];
            
            prevCursor[0] = cursor[0];
            prevCursor[1] = cursor[1];
                
            cursor[0] = mx;
            cursor[1] = my;
              
            return this->exec();
        }
    
        Smudge(int w,int h,NSString *path) : MapRG16UnormBase(w, h, path) {
        
            this->_params.push_back([this->device newBufferWithLength:sizeof(float)*2 options:MTLResourceOptionCPUCacheModeDefault]); // prevCursor
            this->_params.push_back([this->device newBufferWithLength:sizeof(float)*2 options:MTLResourceOptionCPUCacheModeDefault]); // cursor
        }
            
        ~Smudge() {
            
        }
};
