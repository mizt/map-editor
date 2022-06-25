#include "AEConfig.h"
#include "entry.h"
#include "AE_Effect.h"
#include "A.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"

enum {
    MAP_INPUT = 0,
    MAP_LAYER,
    MAP_BILINEAR,
    MAP_X,
    MAP_Y,
    MAP_NUM_PARAMS
};

static PF_Err GlobalSetup(PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef *output) {
    
    PF_Err err = PF_Err_NONE;
    
    out_data->my_version = PF_VERSION(2,0,0,PF_Stage_DEVELOP,1);
    out_data->out_flags = PF_OutFlag_WIDE_TIME_INPUT;
    out_data->out_flags2 = PF_OutFlag2_SUPPORTS_THREADED_RENDERING;

    in_data->sequence_data = out_data->sequence_data = nullptr;
    
    return err;
}

static PF_Err ParamsSetup(PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef *output) {
    
    PF_Err err = PF_Err_NONE;
    PF_ParamDef def;
    
    AEFX_CLR_STRUCT(def);
    PF_ADD_LAYER("Target",PF_LayerDefault_MYSELF,MAP_LAYER);
    
    AEFX_CLR_STRUCT(def);
    PF_ADD_CHECKBOX("Bilinear","ON",TRUE,0,MAP_BILINEAR);
    
    AEFX_CLR_STRUCT(def);
    PF_ADD_SLIDER("X",0,100,0,100,100,MAP_X);
    
    AEFX_CLR_STRUCT(def);
    PF_ADD_SLIDER("Y",0,100,0,100,100,MAP_Y);
    
    out_data->num_params = MAP_NUM_PARAMS;
    
    return err;
}

static PF_Err SequenceSetup(PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef *output) {
    
    return PF_Err_NONE;
}

static PF_Err SequenceSetdown(PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef *output) {
    
    return PF_Err_NONE;
}

static PF_Err SequenceResetup(PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef *output) {
    
    return PF_Err_NONE;
}

static PF_Err Render(PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef *output) {
    
    PF_ParamDef param;
    AEFX_CLR_STRUCT(param);
    
    bool isBilinear = params[MAP_BILINEAR]->u.bd.value;

    float sx = params[MAP_X]->u.sd.value*0.01;
    float sy = params[MAP_Y]->u.sd.value*0.01;

    double scaleX = (double)in_data->downsample_x.num/(double)in_data->downsample_x.den;
    double scaleY = (double)in_data->downsample_y.num/(double)in_data->downsample_y.den;
    
    PF_Err err = in_data->inter.checkout_param((in_data)->effect_ref,MAP_LAYER,in_data->current_time,in_data->time_step,in_data->time_scale,&param);
                            
    if(!err) {
        
        unsigned int *dst = (unsigned int *)output->data;
        int dstWidth = output->width;
        int dstHeight = output->height;
        int dstRow = output->rowbytes>>2;
        
        PF_EffectWorld *input = &params[MAP_INPUT]->u.ld;
        unsigned int *src = (unsigned int *)input->data;
        int srcRow = input->rowbytes>>2;
        int srcWidth = input->width;
        int srcHeight = input->height;
        
        int type = 0;
        if(param.u.ld.data&&(dstWidth==srcWidth)&&(dstHeight==srcHeight)) {
            PF_EffectWorld *data = &param.u.ld;
            type = ((srcWidth==data->width)&&(srcHeight==data->height))?1:0;
        }
        
        if(type==1) {
            
            PF_EffectWorld *data = &param.u.ld;
            unsigned int *map = (unsigned int *)data->data;
            int mapRow = data->rowbytes>>2;
            
            if(!isBilinear) {
                
                for(int i=0; i<dstHeight; i++) {
                    for(int j=0; j<dstWidth; j++) {
                        
                        dst[i*dstRow+j] = 0x00000000;
                        
                        unsigned int xy = map[i*mapRow+j];
                        
                        unsigned char a = (xy)&0xFF;
                        unsigned char r = (xy>>8)&0xFF;
                        unsigned char g = (xy>>16)&0xFF;
                        unsigned char b = (xy>>24)&0xFF;
                               
                        float x = ((((a<<8|b)-0x5555)*0.125))*scaleX;
                        float y = ((((g<<8|r)-0x5555)*0.125))*scaleY;
                                                    
                        int x2 = (sx>1.0)?x*(1.0/sx):j*(1.0-sx)+x*sx;
                        int y2 = (sy>1.0)?y*(1.0/sy):i*(1.0-sy)+y*sy;
                       
                        if(x2>=0&&x2<srcWidth&&y2>=0&&y2<srcHeight) {
                            dst[i*dstRow+j] = src[y2*srcRow+x2];
                        }
                    }
                }
            }
            else {
                
                for(int i=0; i<dstHeight; i++) {
                    for(int j=0; j<dstWidth; j++) {
                        
                        dst[i*dstRow+j] = 0x00000000;
                                                    
                        unsigned int xy = map[i*mapRow+j];
                        
                        unsigned char a = (xy)&0xFF;
                        unsigned char r = (xy>>8)&0xFF;
                        unsigned char g = (xy>>16)&0xFF;
                        unsigned char b = (xy>>24)&0xFF;
                               
                        float x = ((((a<<8|b)-0x5555)*0.125))*scaleX;
                        float y = ((((g<<8|r)-0x5555)*0.125))*scaleY;
                        
                        float x2 = (sx>1.0)?x*(1.0/sx):j*(1.0-sx)+x*sx;
                        float y2 = (sy>1.0)?y*(1.0/sy):i*(1.0-sy)+y*sy;
                        
                        int ix2 = x2;
                        int iy2 = y2;
                        
                        if(ix2>=0&&ix2<srcWidth&&iy2>=0&&iy2<srcHeight) {
                            
                            uint32 dx = (x2-ix2)*16.0;
                            uint32 dy = (y2-iy2)*16.0;
                            
                            unsigned int *s = src+(iy2*srcRow+ix2);
                            
                            uint32 s1 = *s;
                            if(dx) s++;
                            uint32 s2 = *s;
                            if(dy) s+=srcRow;
                            uint32 s3 = *s;
                            if(dx) s--;
                            uint32 s4 = *s;
                            
                            uint32 w1 = (0x10-dx)*(0x10-dy);
                            uint32 w2 = dx*(0x10-dy);
                            uint32 w3 = dx*dy;
                            uint32 w4 = (0x10-dx)*dy;
                            
                            uint32 a = ((s1)&0xFF)*w1;
                            a+=((s2)&0xFF)*w2;
                            a+=((s3)&0xFF)*w3;
                            a+=((s4)&0xFF)*w4;
                            a&=0xFF00;
                            
                            uint32 r = ((s1>>8)&0xFF)*w1;
                            r+=((s2>>8)&0xFF)*w2;
                            r+=((s3>>8)&0xFF)*w3;
                            r+=((s4>>8)&0xFF)*w4;
                            r&=0xFF00;
                            
                            uint32 g = ((s1>>16)&0xFF)*w1;
                            g+=((s2>>16)&0xFF)*w2;
                            g+=((s3>>16)&0xFF)*w3;
                            g+=((s4>>16)&0xFF)*w4;
                            g&=0xFF00;
                            
                            uint32 b = ((s1>>24)&0xFF)*w1;
                            b+=((s2>>24)&0xFF)*w2;
                            b+=((s3>>24)&0xFF)*w3;
                            b+=((s4>>24)&0xFF)*w4;
                            b&=0xFF00;
                            
                            dst[i*dstRow+j] = (b<<16|g<<8|r)|(a>>8);
                        }
                    }
                }
            }
        }
        else {
            
            for(int i=0; i<dstHeight; i++) {
                for(int j=0; j<dstWidth; j++) {
                    dst[i*dstRow+j] = 0xFF0000FF;
                }
            }
        }
    }

    ERR(PF_CHECKIN_PARAM(in_data,&param)); // ALWAYS check in, even if invalid param.
    return err;
}

extern "C" DllExport PF_Err EffectMain(PF_Cmd cmd, PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef *output) {
    
    PF_Err err = PF_Err_NONE;
    
    switch (cmd) {
        case PF_Cmd_GLOBAL_SETUP: err = GlobalSetup(in_data,out_data,params,output); break;
        case PF_Cmd_PARAMS_SETUP: err = ParamsSetup(in_data,out_data,params,output); break;
        case PF_Cmd_SEQUENCE_SETUP: err = SequenceSetup(in_data,out_data,params,output); break;
        case PF_Cmd_SEQUENCE_SETDOWN: err = SequenceSetdown(in_data,out_data,params,output); break;
        case PF_Cmd_SEQUENCE_RESETUP: err = SequenceResetup(in_data,out_data,params,output); break;
        case PF_Cmd_RENDER: err = Render(in_data,out_data,params,output); break;
    }
    return err;
}
