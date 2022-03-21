typedef short PLANE_INDICES_TYPE;

class Plane {
    
    public:
        
        constexpr static int TEXCOORD_SIZE = 4*2;
        constexpr static float texcoord[TEXCOORD_SIZE] = {
            0.f, 0.f,
            1.f, 0.f,
            1.f, 1.f,
            0.f, 1.f
        };
        
        constexpr static int VERTICES_SIZE = 4*4;
        constexpr static float vertices[VERTICES_SIZE] = {
            -1.f,-1.f, 0.f, 1.f,
             1.f,-1.f, 0.f, 1.f,
             1.f, 1.f, 0.f, 1.f,
            -1.f, 1.f, 0.f, 1.f
        };
        
        constexpr static int INDICES_TYPE = sizeof(PLANE_INDICES_TYPE);
        constexpr static int INDICES_SIZE = 3*2;
        constexpr static PLANE_INDICES_TYPE indices[INDICES_SIZE] = {
            0,1,2,
            0,2,3
        };
        
        Plane() {}
};

