#include "engine_physics.h"
#include "debug/debug_print.h"
#include "nodes/2D/physics_2d_node.h"
#include "math/vector2.h"
#include "math/engine_math.h"
#include "physics/shapes/shape_test.h"



// A linked list of physics nodes and manifolds to loop
// through quickly
static const linked_list empty_list = (linked_list){NULL, NULL, 0, 0};
linked_list engine_physics_nodes = empty_list;
linked_list engine_physics_manifolds = empty_list;
float gravity = 0.00981f;

void engine_physics_init(){
    linked_list_init(&engine_physics_nodes);
    linked_list_init(&engine_physics_manifolds);
}


void engine_physics_tick(){
    // Update all node positions
    linked_list_node *current_physics_node = engine_physics_nodes.start;
    while(current_physics_node != NULL){

        engine_node_base_t *node_base = current_physics_node->object;
        
        vector2_class_obj_t *vel = mp_load_attr(node_base->attr_accessor, MP_QSTR_velocity);
        vector2_class_obj_t *pos = mp_load_attr(node_base->attr_accessor, MP_QSTR_position);
        mp_float_t rotation = mp_obj_get_float(mp_load_attr(node_base->attr_accessor, MP_QSTR_rotation));
        mp_float_t angular_vel = mp_obj_get_float(mp_load_attr(node_base->attr_accessor, MP_QSTR_angular_velocity));
        vector2_class_obj_t *acceleration = mp_load_attr(node_base->attr_accessor, MP_QSTR_acceleration);

        pos->x += vel->x;
        pos->y += vel->y;

        vel->y += gravity+acceleration->y;

        mp_store_attr(node_base->attr_accessor, MP_QSTR_rotation, mp_obj_new_float(rotation+angular_vel));
        mp_store_attr(node_base->attr_accessor, MP_QSTR_angular_velocity, mp_obj_new_float(0));

        current_physics_node = current_physics_node->next;
    }

    ENGINE_INFO_PRINTF("Clearing manifolds, list length is %d\n\r", engine_physics_manifolds.count);
    // Clear out the manifolds
    linked_list_node *node_m = engine_physics_manifolds.start;
    int manifolds = 0;
    while(node_m != NULL) {
        linked_list_node *next = node_m->next;
        linked_list_del_list_node(&engine_physics_manifolds, node_m);
        ENGINE_INFO_PRINTF("Deleted manifold\n\r\n\r");
        node_m = next;
        manifolds++;
    }

    ENGINE_INFO_PRINTF("Cleared %f manifolds, Generating manifolds...\n\r", (float)manifolds);
    manifolds = 0;
    // Generate manifolds
    ENGINE_INFO_PRINTF("Physics node list length is %d\n\r", engine_physics_nodes.count);
    linked_list_node *node_a = engine_physics_nodes.start;
    while(node_a != NULL){

        //engine_node_base_t *a_physics_node_base = node_a->object;
        linked_list_node *node_b = node_a->next;

        while(node_b != NULL){

            mp_obj_t m = physics_2d_node_class_test(node_a->object, node_b->object);

            ((physics_manifold_class_obj_t*)MP_OBJ_TO_PTR(m))->body_a = node_a->object;
            ((physics_manifold_class_obj_t*)MP_OBJ_TO_PTR(m))->body_b = node_b->object;

            linked_list_add_obj(&engine_physics_manifolds, MP_OBJ_TO_PTR(m));
            manifolds++;

            node_b = node_b->next;
        }
        node_a = node_a->next;
    }

    ENGINE_INFO_PRINTF("Generated %f manifolds\n\r", (float)manifolds);

    ENGINE_INFO_PRINTF("Applying manifolds\n\r");
    // Apply manifold impulses
    node_m = engine_physics_manifolds.start;
    while(node_m != NULL) {
        physics_manifold_class_obj_t* m = MP_OBJ_TO_PTR(node_m->object);
        ENGINE_INFO_PRINTF("Applying manifold %p\n\r", m);
        physics_2d_node_class_apply_manifold_impulse(m->body_a, m->body_b, MP_OBJ_FROM_PTR(m));
        node_m = node_m->next;
    }
}


linked_list_node *engine_physics_track_node(engine_node_base_t *obj){
    ENGINE_INFO_PRINTF("Tracking physics node %p", obj);
    return linked_list_add_obj(&engine_physics_nodes, obj);
}


void engine_physics_untrack_node(linked_list_node *physics_list_node){
    ENGINE_INFO_PRINTF("Untracking physics node");
    linked_list_del_list_node(&engine_physics_nodes, physics_list_node);
}


void engine_physics_clear_all(){
    ENGINE_INFO_PRINTF("Untracking physics nodes...");
    linked_list_clear(&engine_physics_nodes);
}