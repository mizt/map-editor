typedef short MESH_INDICES_TYPE;

class Mesh {
    
    private:
        
        unsigned int w = 0;
        unsigned int h = 0;
    
    public:
        
        int TEXCOORD_SIZE = 0;
        float *texcoord = nullptr;
        
        int VERTICES_SIZE = 0;
        float *vertices = nullptr;
        
        int INDICES_TYPE = sizeof(MESH_INDICES_TYPE);
        int INDICES_SIZE = 0;
        MESH_INDICES_TYPE *indices = nullptr;
    
        int width() {
            return this->w;
        }
    
        int height() {
            return this->h;
        }
            
        Mesh(unsigned int x=16, unsigned int y=9) {
         
            if(x<2) x = 2;
            if(y<2) y = 2;
            
            this->w = x;
            this->h = y;
                                    
            this->VERTICES_SIZE = (x*y)<<2;
            this->vertices = new float[this->VERTICES_SIZE];
             for(int i=0; i<y; i++) {
                for(int j=0; j<x; j++) {
                        
                    int addr = (i*x+j)<<2;

                    this->vertices[addr+0] = (j/((float)(x-1)))*2.0-1.0;
                    this->vertices[addr+1] = (i/((float)(y-1)))*2.0-1.0;
                    this->vertices[addr+2] = 0;
                    this->vertices[addr+3] = 1;
                }
            }
            
            this->INDICES_SIZE = ((x-1)*(y-1))*6;
            this->indices = new MESH_INDICES_TYPE[this->INDICES_SIZE];
            for(int i=0; i<y-1; i++) {
                for(int j=0; j<x-1; j++) {
                    
                    int addr = (i*(x-1)+j)*6;
                    int o = i*x;

                    this->indices[addr+0] = o+j;
                    this->indices[addr+1] = o+(j+x+1);
                    this->indices[addr+2] = o+(j+x);
                
                    this->indices[addr+3] = o+j;
                    this->indices[addr+4] = o+(j+1);
                    this->indices[addr+5] = o+(j+x+1);
                }
            }
            
            this->TEXCOORD_SIZE = (x*y)<<1;
            this->texcoord = new float[this->TEXCOORD_SIZE];
            for(int i=0; i<y; i++) {
                for(int j=0; j<x; j++) {
                    int addr = (i*x+j)<<1;
                    this->texcoord[addr+0] = (j/(float)(x-1));
                    this->texcoord[addr+1] = (i/(float)(y-1));
                }
            }
            
        }
};
