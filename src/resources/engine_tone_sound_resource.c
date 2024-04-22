#include "engine_tone_sound_resource.h"
#include "audio/engine_audio_channel.h"
#include "audio/engine_audio_module.h"
#include "debug/debug_print.h"
#include "resources/engine_resource_manager.h"
#include "math/engine_math.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>


#include "../lib/cglm/include/cglm/util.h"


float ENGINE_FAST_FUNCTION(tone_sound_resource_get_sample)(tone_sound_resource_class_obj_t *self){
    float omega = 0.0f;

    if(self->next_frequency_ready){

        omega = glm_lerp(self->omega, self->next_omega, self->interpolation);

        self->interpolation += 0.001f;

        if(self->interpolation >= 1.0f){
            self->interpolation = 0.0f;
            self->next_frequency_ready = false;
            self->frequency = self->next_frequency;
            self->omega = self->next_omega;
        }
    }else{
        omega = self->omega;
    }

    float sample = engine_math_fast_sin(omega * self->time);
    self->time += ENGINE_AUDIO_SAMPLE_DT;
    return sample;
}


mp_obj_t tone_sound_resource_class_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args){
    ENGINE_INFO_PRINTF("New ToneSoundResource");
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    tone_sound_resource_class_obj_t *self = m_new_obj_with_finaliser(tone_sound_resource_class_obj_t);
    self->base.type = &tone_sound_resource_class_type;
    self->channel = NULL;

    // https://www.mathworks.com/matlabcentral/answers/36428-sine-wave-plot#answer_45572
    self->frequency = 1000.0f;
    self->omega = 2.0f * PI * self->frequency;
    self->time = 0.0f;

    self->next_frequency = 0.0f;
    self->next_omega = 0.0f;
    self->next_frequency_ready = false;
    self->interpolation = 0.0f;

    return MP_OBJ_FROM_PTR(self);
}


// Class methods
STATIC mp_obj_t tone_sound_resource_class_del(mp_obj_t self_in){
    ENGINE_INFO_PRINTF("ToneSoundResource: Deleted (freeing sound data)");

    tone_sound_resource_class_obj_t *self = self_in;
    audio_channel_class_obj_t *channel = self->channel;

    // This is very important! Need to make sure to set channel source this source is
    // related to NULL. Otherwise, even though this source gets collected it will not
    // be set to NULL and the audio ISR will try to access invalid memory!!!
    if(channel != NULL){
        audio_channel_stop(channel);
    }

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(tone_sound_resource_class_del_obj, tone_sound_resource_class_del);


/*  --- doc ---
    NAME: ToneSoundResource
    DESC: Can be used to play a tone on an audio channel
    ATTR:   [type=float]    [name=frequency]    [value=any]                                                                                                                                                                  
*/ 
STATIC void tone_sound_resource_class_attr(mp_obj_t self_in, qstr attribute, mp_obj_t *destination){
    ENGINE_INFO_PRINTF("Accessing ToneSoundResource attr");

    tone_sound_resource_class_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if(destination[0] == MP_OBJ_NULL){          // Load
        switch(attribute){
            case MP_QSTR___del__:
                destination[0] = MP_OBJ_FROM_PTR(&tone_sound_resource_class_del_obj);
                destination[1] = self_in;
            break;
            case MP_QSTR_frequency:
                destination[0] = mp_obj_new_float(self->frequency);
            break;
            default:
                return; // Fail
        }
    }else if(destination[1] != MP_OBJ_NULL){    // Store
        switch(attribute){
            case MP_QSTR_frequency:
            {
                if(self->channel == NULL){
                    self->frequency = mp_obj_get_float(destination[1]);
                    self->omega = 2.0f * PI * self->frequency;
                    self->next_frequency_ready = false;
                }else{
                    self->next_frequency = mp_obj_get_float(destination[1]);
                    self->next_omega = 2.0f * PI * self->next_frequency;
                    self->next_frequency_ready = true;
                }
            }
            break;
            default:
                return; // Fail
        }

        // Success
        destination[0] = MP_OBJ_NULL;
    }
}


// Class attributes
STATIC const mp_rom_map_elem_t tone_sound_resource_class_locals_dict_table[] = {

};
STATIC MP_DEFINE_CONST_DICT(tone_sound_resource_class_locals_dict, tone_sound_resource_class_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    tone_sound_resource_class_type,
    MP_QSTR_ToneSoundResource,
    MP_TYPE_FLAG_NONE,

    make_new, tone_sound_resource_class_new,
    attr, tone_sound_resource_class_attr,
    locals_dict, &tone_sound_resource_class_locals_dict
);