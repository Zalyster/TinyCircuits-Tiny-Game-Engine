#include "text_2d_node.h"

#include "py/objstr.h"
#include "py/objtype.h"
#include "nodes/node_types.h"
#include "debug/debug_print.h"
#include "engine_object_layers.h"
#include "math/vector2.h"
#include "math/vector3.h"
#include "math/rectangle.h"
#include "draw/engine_display_draw.h"
#include "resources/engine_texture_resource.h"
#include "resources/engine_font_resource.h"
#include "utility/engine_file.h"
#include "math/engine_math.h"
#include "utility/engine_time.h"
#include "draw/engine_shader.h"


void text_2d_node_class_draw(engine_node_base_t *text_2d_node_base, mp_obj_t camera_node){
    ENGINE_INFO_PRINTF("Text2DNode: Drawing");

    engine_text_2d_node_class_obj_t *text_2d_node = text_2d_node_base->node;

    // Very first thing is to early out if the text is not set
    if(text_2d_node->text == mp_const_none){
        return mp_const_none;
    }

    engine_node_base_t *camera_node_base = camera_node;

    vector2_class_obj_t *text_scale =  text_2d_node->scale;
    float text_opacity = mp_obj_get_float(text_2d_node->opacity);

    float text_box_width = mp_obj_get_float(text_2d_node->width);
    float text_box_height = mp_obj_get_float(text_2d_node->height);

    vector3_class_obj_t *camera_position = mp_load_attr(camera_node_base->attr_accessor, MP_QSTR_position);
    rectangle_class_obj_t *camera_viewport = mp_load_attr(camera_node_base->attr_accessor, MP_QSTR_viewport);
    float camera_zoom = mp_obj_get_float(mp_load_attr(camera_node_base->attr_accessor, MP_QSTR_zoom));

    float camera_resolved_hierarchy_x = 0.0f;
    float camera_resolved_hierarchy_y = 0.0f;
    float camera_resolved_hierarchy_rotation = 0.0f;
    node_base_get_child_absolute_xy(&camera_resolved_hierarchy_x, &camera_resolved_hierarchy_y, &camera_resolved_hierarchy_rotation, NULL, camera_node);
    camera_resolved_hierarchy_rotation = -camera_resolved_hierarchy_rotation;

    float text_resolved_hierarchy_x = 0.0f;
    float text_resolved_hierarchy_y = 0.0f;
    float text_resolved_hierarchy_rotation = 0.0f;
    bool text_is_child_of_camera = false;
    node_base_get_child_absolute_xy(&text_resolved_hierarchy_x, &text_resolved_hierarchy_y, &text_resolved_hierarchy_rotation, &text_is_child_of_camera, text_2d_node_base);

    // Store the non-rotated x and y for a second
    float text_rotated_x = text_resolved_hierarchy_x - camera_resolved_hierarchy_x;
    float text_rotated_y = text_resolved_hierarchy_y - camera_resolved_hierarchy_y;

    if(text_is_child_of_camera == false){
        // Scale transformation due to camera zoom
        engine_math_scale_point(&text_rotated_x, &text_rotated_y, camera_position->x.value, camera_position->y.value, camera_zoom);
    }else{
        camera_zoom = 1.0f;
    }

    // Rotate text origin about the camera
    engine_math_rotate_point(&text_rotated_x, &text_rotated_y, 0, 0, camera_resolved_hierarchy_rotation);

    text_rotated_x += camera_viewport->width/2;
    text_rotated_y += camera_viewport->height/2;

    // Decide which shader to use per-pixel
    engine_shader_t *shader = &empty_shader;
    if(text_opacity < 1.0f){
        shader = &opacity_shader;
    }

    engine_draw_text(text_2d_node->font_resource, text_2d_node->text, text_rotated_x, text_rotated_y, text_box_width, text_box_height, text_scale->x.value*camera_zoom, text_scale->y.value*camera_zoom, text_resolved_hierarchy_rotation+camera_resolved_hierarchy_rotation, text_opacity, shader);
}


// `native`     == instance of this built-in type (`Text2DNode`)
// `not native` == instance of a Python class that inherits this built-in type (`Text2DNode`)
STATIC void text_2d_node_class_calculate_dimensions(engine_text_2d_node_class_obj_t *text_2d_node){
    // Get the text and early out if none set
    if(text_2d_node->text == mp_const_none || text_2d_node->font_resource == mp_const_none){
        text_2d_node->width = mp_obj_new_int(0);
        text_2d_node->height = mp_obj_new_int(0);
        return;
    }

    float text_box_width = 0.0f;
    float text_box_height = 0.0f;
    font_resource_get_box_dimensions(text_2d_node->font_resource, text_2d_node->text, &text_box_width, &text_box_height);

    text_2d_node->width = mp_obj_new_int((uint32_t)text_box_width);
    text_2d_node->height = mp_obj_new_int((uint32_t)text_box_height);
}


// Return `true` if handled loading the attr from internal structure, `false` otherwise
bool text_2d_node_load_attr(engine_node_base_t *self_node_base, qstr attribute, mp_obj_t *destination){
    // Get the underlying structure
    engine_text_2d_node_class_obj_t *self = self_node_base->node;

    switch(attribute){
        case MP_QSTR___del__:
            destination[0] = MP_OBJ_FROM_PTR(&node_base_del_obj);
            destination[1] = self_node_base;
            return true;
        break;
        case MP_QSTR_add_child:
            destination[0] = MP_OBJ_FROM_PTR(&node_base_add_child_obj);
            destination[1] = self_node_base;
            return true;
        break;
        case MP_QSTR_get_child:
            destination[0] = MP_OBJ_FROM_PTR(&node_base_get_child_obj);
            destination[1] = self_node_base;
            return true;
        break;
        case MP_QSTR_remove_child:
            destination[0] = MP_OBJ_FROM_PTR(&node_base_remove_child_obj);
            destination[1] = self_node_base;
            return true;
        break;
        case MP_QSTR_set_layer:
            destination[0] = MP_OBJ_FROM_PTR(&node_base_set_layer_obj);
            destination[1] = self_node_base;
            return true;
        break;
        case MP_QSTR_get_layer:
            destination[0] = MP_OBJ_FROM_PTR(&node_base_get_layer_obj);
            destination[1] = self_node_base;
            return true;
        break;
        case MP_QSTR_tick:
            destination[0] = MP_OBJ_FROM_PTR(&node_base_get_layer_obj);
            destination[1] = self_node_base->attr_accessor;
            return true;
        break;
        case MP_QSTR_node_base:
            destination[0] = self_node_base;
            return true;
        break;
        case MP_QSTR_position:
            destination[0] = self->position;
            return true;
        break;
        case MP_QSTR_font:
            destination[0] = self->font_resource;
            return true;
        break;
        case MP_QSTR_text:
            destination[0] = self->text;
            return true;
        break;
        case MP_QSTR_rotation:
            destination[0] = self->rotation;
            return true;
        break;
        case MP_QSTR_scale:
            destination[0] = self->scale;
            return true;
        break;
        case MP_QSTR_opacity:
            destination[0] = self->opacity;
            return true;
        break;
        case MP_QSTR_width:
            destination[0] = self->width;
            return true;
        break;
        case MP_QSTR_height:
            destination[0] = self->height;
            return true;
        break;
        default:
            return false; // Fail
    }
}


// Return `true` if handled storing the attr from internal structure, `false` otherwise
bool text_2d_node_store_attr(engine_node_base_t *self_node_base, qstr attribute, mp_obj_t *destination){
    // Get the underlying structure
    engine_text_2d_node_class_obj_t *self = self_node_base->node;

    switch(attribute){
        case MP_QSTR_tick:
            self->tick_cb = destination[1];
            return true;
        break;
        case MP_QSTR_position:
            self->position = destination[1];
            return true;
        break;
        case MP_QSTR_font:
            self->font_resource = destination[1];
            text_2d_node_class_calculate_dimensions(self);
            return true;
        break;
        case MP_QSTR_text:
            self->text = destination[1];
            text_2d_node_class_calculate_dimensions(self);
            return true;
        break;
        case MP_QSTR_rotation:
            self->rotation = destination[1];
            return true;
        break;
        case MP_QSTR_scale:
            self->scale = destination[1];
            return true;
        break;
        case MP_QSTR_opacity:
            self->opacity = destination[1];
            return true;
        break;
        case MP_QSTR_width:
            mp_raise_msg(&mp_type_AttributeError, MP_ERROR_TEXT("Text2DNode: ERROR: 'width' is read-only, it is not allowed to be set!"));
            return true;
        break;
        case MP_QSTR_height:
            mp_raise_msg(&mp_type_AttributeError, MP_ERROR_TEXT("Text2DNode: ERROR: 'height' is read-only, it is not allowed to be set!"));
            return true;
        break;
        default:
            return false; // Fail
    }
}


STATIC mp_attr_fun_t text_2d_node_class_attr(mp_obj_t self_in, qstr attribute, mp_obj_t *destination){
    ENGINE_INFO_PRINTF("Accessing Text2DNode attr");

    // Get the node base from either class
    // instance or native instance object
    bool is_obj_instance = false;
    engine_node_base_t *node_base = node_base_get(self_in, &is_obj_instance);

    // Used for telling if custom load/store functions handled the attr
    bool attr_handled = false;

    if(destination[0] == MP_OBJ_NULL){          // Load
        attr_handled = text_2d_node_load_attr(node_base, attribute, destination);
    }else if(destination[1] != MP_OBJ_NULL){    // Store
        attr_handled = text_2d_node_store_attr(node_base, attribute, destination);

        // If handled, mark as successful store
        if(attr_handled) destination[0] = MP_OBJ_NULL;
    }

    // If this is a Python class instance and the attr was NOT
    // handled by the above, defer the attr to the instance attr
    // handler
    if(is_obj_instance && attr_handled == false){
        default_instance_attr_func(self_in, attribute, destination);
    }

    return mp_const_none;
}




/*  --- doc ---
    NAME: Text2DNode
    DESC: Simple 2D sprite node that displays text
    PARAM:  [type={ref_link:Vector2}]         [name=position]                   [value={ref_link:Vector2}]
    PARAM:  [type={ref_link:FontResource}]    [name=font]                       [value={ref_link:FontResource}]
    PARAM:  [type=string]                     [name=text]                       [value=any]
    PARAM:  [type=float]                      [name=rotation]                   [value=any (radians)]
    PARAM:  [type={ref_link:Vector2}]         [name=scale]                      [value={ref_link:Vector2}]
    PARAM:  [type=float]                      [name=opacity]                    [value=0 ~ 1.0]
    ATTR:   [type=function]                   [name={ref_link:add_child}]       [value=function] 
    ATTR:   [type=function]                   [name={ref_link:get_child}]       [value=function] 
    ATTR:   [type=function]                   [name={ref_link:remove_child}]    [value=function]
    ATTR:   [type=function]                   [name={ref_link:set_layer}]       [value=function]
    ATTR:   [type=function]                   [name={ref_link:get_layer}]       [value=function]
    ATTR:   [type=function]                   [name={ref_link:remove_child}]    [value=function]
    ATTR:   [type=function]                   [name={ref_link:tick}]            [value=function]
    ATTR:   [type={ref_link:Vector2}]         [name=position]                   [value={ref_link:Vector2}]
    ATTR:   [type={ref_link:FontResource}]    [name=font]                       [value={ref_link:FontResource}]
    ATTR:   [type=string]                     [name=text]                       [value=any]
    ATTR:   [type=float]                      [name=rotation]                   [value=any (radians)]
    ATTR:   [type={ref_link:Vector2}]         [name=scale]                      [value={ref_link:Vector2}]
    ATTR:   [type=float]                      [name=opacity]                    [value=0 ~ 1.0]
    OVRR:   [type=function]                   [name={ref_link:tick}]            [value=function]
*/
mp_obj_t text_2d_node_class_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args){
    ENGINE_INFO_PRINTF("New Text2DNode");

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_child_class,          MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_position,             MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_font,                 MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_text,                 MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_rotation,             MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_scale,                MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_opacity,              MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    };
    mp_arg_val_t parsed_args[MP_ARRAY_SIZE(allowed_args)];
    enum arg_ids {child_class, position, font, text, rotation, scale, opacity};
    bool inherited = false;

    // If there is one positional argument and it isn't the first 
    // expected argument (as is expected when using positional
    // arguments) then define which way to parse the arguments
    if(n_args >= 1 && mp_obj_get_type(args[0]) != &vector2_class_type){
        // Using positional arguments but the type of the first one isn't
        // as expected. Must be the child class
        mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed_args);
        inherited = true;
    }else{
        // Whether we're using positional arguments or not, prase them this
        // way. It's a requirement that the child class be passed using position.
        // Adjust what and where the arguments are parsed, since not inherited based
        // on the first argument
        mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(allowed_args)-1, allowed_args+1, parsed_args+1);
        inherited = false;
    }

    if(parsed_args[position].u_obj == MP_OBJ_NULL) parsed_args[position].u_obj = vector2_class_new(&vector2_class_type, 0, 0, NULL);
    if(parsed_args[font].u_obj == MP_OBJ_NULL) parsed_args[font].u_obj = mp_const_none;
    if(parsed_args[text].u_obj == MP_OBJ_NULL) parsed_args[text].u_obj = mp_const_none;
    if(parsed_args[rotation].u_obj == MP_OBJ_NULL) parsed_args[rotation].u_obj = mp_obj_new_float(0.0f);
    if(parsed_args[scale].u_obj == MP_OBJ_NULL) parsed_args[scale].u_obj = vector2_class_new(&vector2_class_type, 2, 0, (mp_obj_t[]){mp_obj_new_float(1.0f), mp_obj_new_float(1.0f)});
    if(parsed_args[opacity].u_obj == MP_OBJ_NULL) parsed_args[opacity].u_obj = mp_obj_new_float(1.0f);

    // All nodes are a engine_node_base_t node. Specific node data is stored in engine_node_base_t->node
    engine_node_base_t *node_base = m_new_obj_with_finaliser(engine_node_base_t);
    node_base_init(node_base, &engine_text_2d_node_class_type, NODE_TYPE_TEXT_2D);
    engine_text_2d_node_class_obj_t *text_2d_node = m_malloc(sizeof(engine_text_2d_node_class_obj_t));
    node_base->node = text_2d_node;
    node_base->attr_accessor = node_base;

    text_2d_node->tick_cb = mp_const_none;
    text_2d_node->position = parsed_args[position].u_obj;
    text_2d_node->font_resource = parsed_args[font].u_obj;
    text_2d_node->text = parsed_args[text].u_obj;
    text_2d_node->rotation = parsed_args[rotation].u_obj;
    text_2d_node->scale = parsed_args[scale].u_obj;
    text_2d_node->opacity = parsed_args[opacity].u_obj;
    text_2d_node->width = mp_obj_new_int(0);
    text_2d_node->height = mp_obj_new_int(0);

    text_2d_node_class_calculate_dimensions(text_2d_node);

    if(inherited == true){  // Inherited (use existing object)
        // Get the Python class instance
        mp_obj_t node_instance = parsed_args[child_class].u_obj;

        // Because the instance doesn't have a `node_base` yet, restore the
        // instance type original attr function for now (otherwise get core abort)
        if(default_instance_attr_func != NULL) MP_OBJ_TYPE_SET_SLOT((mp_obj_type_t*)((mp_obj_base_t*)node_instance)->type, attr, default_instance_attr_func, 5);

        // Look for function overrides otherwise use the defaults
        mp_obj_t dest[2];

        mp_load_method_maybe(node_base->node, MP_QSTR_tick, dest);
        if(dest[0] == MP_OBJ_NULL && dest[1] == MP_OBJ_NULL){   // Did not find method (set to default)
            text_2d_node->tick_cb = mp_const_none;
        }else{                                                  // Likely found method (could be attribute)
            text_2d_node->tick_cb = dest[0];
        }

        // Store one pointer on the instance. Need to be able to get the
        // node base that contains a pointer to the engine specific data we
        // care about
        // mp_store_attr(node_instance, MP_QSTR_node_base, node_base);
        mp_store_attr(node_instance, MP_QSTR_node_base, node_base);

        // Store default Python class instance attr function
        // and override with custom intercept attr function
        // so that certain callbacks/code can run (see py/objtype.c:mp_obj_instance_attr(...))
        default_instance_attr_func = MP_OBJ_TYPE_GET_SLOT((mp_obj_type_t*)((mp_obj_base_t*)node_instance)->type, attr);
        MP_OBJ_TYPE_SET_SLOT((mp_obj_type_t*)((mp_obj_base_t*)node_instance)->type, attr, text_2d_node_class_attr, 5);

        // Need a way to access the object node instance instead of the native type for callbacks (tick, draw, collision)
        node_base->attr_accessor = node_instance;
    }

    return MP_OBJ_FROM_PTR(node_base);
}


// Class attributes
STATIC const mp_rom_map_elem_t text_2d_node_class_locals_dict_table[] = {

};
STATIC MP_DEFINE_CONST_DICT(text_2d_node_class_locals_dict, text_2d_node_class_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    engine_text_2d_node_class_type,
    MP_QSTR_Text2DNode,
    MP_TYPE_FLAG_NONE,

    make_new, text_2d_node_class_new,
    attr, text_2d_node_class_attr,
    locals_dict, &text_2d_node_class_locals_dict
);