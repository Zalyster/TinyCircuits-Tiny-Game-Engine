#include "engine_physics.h"
#include "debug/debug_print.h"
#include "nodes/2d/physics_rectangle_2d_node.h"
#include "nodes/2d/physics_circle_2d_node.h"
#include "math/vector2.h"
#include "math/engine_math.h"
#include "utility/engine_bit_collection.h"
#include "collision_contact_2d.h"
#include "nodes/node_types.h"
#include "py/obj.h"
#include "draw/engine_display_draw.h"
#include "engine_physics_ids.h"
#include "engine_physics_collision.h"

// Bit array/collection to track nodes that have collided. In the `init` function
// this is sized so that the output indices from a simple paring function can fit
// (https://math.stackexchange.com/a/531914)
engine_bit_collection_t collided_physics_nodes;


linked_list engine_physics_nodes;
float engine_physics_gravity_x = 0.0f;
float engine_physics_gravity_y = -0.00981f;

float engine_physics_fps_limit_period_ms = 16.667f;
float engine_physics_fps_time_at_last_tick_ms = 0.0f;


void engine_physics_init(){
    ENGINE_INFO_PRINTF("EnginePhysics: Starting...")
    engine_physics_init_ids();
    engine_bit_collection_create(&collided_physics_nodes, engine_physics_get_pair_index(PHYSICS_ID_MAX, PHYSICS_ID_MAX));
}


void engine_physics_apply_impulses(){
    linked_list_node *physics_link_node = engine_physics_nodes.start;
    while(physics_link_node != NULL){
        engine_node_base_t *physics_node_base = physics_link_node->object;

        bool physics_node_dynamic = mp_obj_get_int(mp_load_attr(physics_node_base->attr_accessor, MP_QSTR_dynamic));

        if(physics_node_dynamic){
            vector2_class_obj_t *physics_node_acceleration = mp_load_attr(physics_node_base->attr_accessor, MP_QSTR_acceleration);
            vector2_class_obj_t *physics_node_velocity = mp_load_attr(physics_node_base->attr_accessor, MP_QSTR_velocity);
            vector2_class_obj_t *physics_node_position = mp_load_attr(physics_node_base->attr_accessor, MP_QSTR_position);
            vector2_class_obj_t *physics_node_gravity_scale = mp_load_attr(physics_node_base->attr_accessor, MP_QSTR_gravity_scale);

            // Modifying these directly is good enough, don't need mp_store_attr even if using classes at main level!
            // Apply the user defined acceleration
            physics_node_velocity->x += physics_node_acceleration->x;
            physics_node_velocity->y += physics_node_acceleration->y;

            // Apply engine gravity (can be modifed by the user)
            physics_node_velocity->x -= engine_physics_gravity_x * physics_node_gravity_scale->x;
            physics_node_velocity->y -= engine_physics_gravity_y * physics_node_gravity_scale->y;

            // Apply velocity to the position
            physics_node_position->x += physics_node_velocity->x;
            physics_node_position->y += physics_node_velocity->y;
        }

        physics_link_node = physics_link_node->next;
    }
}


void engine_physics_tick(){
    // If it is not time to update physics, then don't, otherwise track when physics was updated last
    if(millis() - engine_physics_fps_time_at_last_tick_ms < engine_physics_fps_limit_period_ms){
        return;
    }else{
        engine_physics_fps_time_at_last_tick_ms = millis();
    }

    // Apply impulses/move objects due to physics before
    // checking for collisions. Doing it this way means
    // you don't see when objects are overlapping and moved
    // back (looks more stable)
    engine_physics_apply_impulses();

    // Loop through all nodes and test for collision against
    // all other nodes (not optimized checking of if nodes are
    // even possibly close to each other)
    linked_list_node *physics_link_node_a = engine_physics_nodes.start;
    while(physics_link_node_a != NULL){
        // Now check 'a' against all nodes 'b'
        linked_list_node *physics_link_node_b = engine_physics_nodes.start;

        while(physics_link_node_b != NULL){
            // Make sure we are not checking against ourselves
            if(physics_link_node_a->object != physics_link_node_b->object){
                engine_node_base_t *physics_node_base_a = physics_link_node_a->object;
                engine_node_base_t *physics_node_base_b = physics_link_node_b->object;
                bool (*check_collision)(engine_node_base_t*, engine_node_base_t*, float*, float*, float*, float*, float*) = NULL;
                void (*get_contact)(float, float, float*, float*, void*, void*) = NULL;

                bool physics_node_a_dynamic = false;
                bool physics_node_b_dynamic = false;

                uint8_t physics_node_a_id = 0;
                uint8_t physics_node_b_id = 0;

                vector2_class_obj_t *physics_node_a_position = NULL;
                vector2_class_obj_t *physics_node_b_position = NULL;

                vector2_class_obj_t *physics_node_a_velocity = NULL;
                vector2_class_obj_t *physics_node_b_velocity = NULL;

                mp_obj_t collision_cb_a = MP_OBJ_NULL;
                mp_obj_t collision_cb_b = MP_OBJ_NULL;

                if(physics_node_base_a->type == NODE_TYPE_PHYSICS_RECTANGLE_2D && physics_node_base_b->type == NODE_TYPE_PHYSICS_RECTANGLE_2D){
                    engine_physics_rectangle_2d_node_class_obj_t *physics_rectangle_a = physics_node_base_a->node;
                    engine_physics_rectangle_2d_node_class_obj_t *physics_rectangle_b = physics_node_base_b->node;

                    physics_node_a_dynamic = mp_obj_get_int(physics_rectangle_a->dynamic);
                    physics_node_b_dynamic = mp_obj_get_int(physics_rectangle_b->dynamic);

                    physics_node_a_id = physics_rectangle_a->physics_id;
                    physics_node_b_id = physics_rectangle_b->physics_id;

                    physics_node_a_position = physics_rectangle_a->position;
                    physics_node_b_position = physics_rectangle_b->position;

                    physics_node_a_velocity = physics_rectangle_a->velocity;
                    physics_node_b_velocity = physics_rectangle_b->velocity;

                    check_collision = engine_physics_check_rect_rect_collision;
                    get_contact = engine_physics_rect_rect_get_contact;

                    collision_cb_a = physics_rectangle_a->collision_cb;
                    collision_cb_b = physics_rectangle_b->collision_cb;
                }else if((physics_node_base_a->type == NODE_TYPE_PHYSICS_RECTANGLE_2D && physics_node_base_b->type == NODE_TYPE_PHYSICS_CIRCLE_2D) ||
                         (physics_node_base_a->type == NODE_TYPE_PHYSICS_CIRCLE_2D && physics_node_base_b->type == NODE_TYPE_PHYSICS_RECTANGLE_2D)){
                    
                    // Want `physics_node_base_a` to always be the rectangle
                    // and `physics_node_base_b` to be the circle
                    if(physics_node_base_b->type == NODE_TYPE_PHYSICS_RECTANGLE_2D){
                        engine_node_base_t *temp = physics_node_base_a;
                        physics_node_base_a = physics_node_base_b;
                        physics_node_base_b = temp;
                    }

                    engine_physics_rectangle_2d_node_class_obj_t *physics_rectangle = physics_node_base_a->node;
                    engine_physics_circle_2d_node_class_obj_t *physics_circle = physics_node_base_b->node;
                    
                    physics_node_a_dynamic = mp_obj_get_int(physics_rectangle->dynamic);
                    physics_node_b_dynamic = mp_obj_get_int(physics_circle->dynamic);

                    physics_node_a_id = physics_rectangle->physics_id;
                    physics_node_b_id = physics_circle->physics_id;

                    physics_node_a_position = physics_rectangle->position;
                    physics_node_b_position = physics_circle->position;

                    physics_node_a_velocity = physics_rectangle->velocity;
                    physics_node_b_velocity = physics_circle->velocity;

                    check_collision = engine_physics_check_rect_circle_collision;
                    get_contact = engine_physics_rect_circle_get_contact;

                    collision_cb_a = physics_rectangle->collision_cb;
                    collision_cb_b = physics_circle->collision_cb;
                }else if(physics_node_base_a->type == NODE_TYPE_PHYSICS_CIRCLE_2D && physics_node_base_b->type == NODE_TYPE_PHYSICS_CIRCLE_2D){
                    engine_physics_circle_2d_node_class_obj_t *physics_circle_a = physics_node_base_a->node;
                    engine_physics_circle_2d_node_class_obj_t *physics_circle_b = physics_node_base_b->node;

                    physics_node_a_dynamic = mp_obj_get_int(physics_circle_a->dynamic);
                    physics_node_b_dynamic = mp_obj_get_int(physics_circle_b->dynamic);

                    physics_node_a_id = physics_circle_a->physics_id;
                    physics_node_b_id = physics_circle_b->physics_id;

                    physics_node_a_position = physics_circle_a->position;
                    physics_node_b_position = physics_circle_b->position;

                    physics_node_a_velocity = physics_circle_a->velocity;
                    physics_node_b_velocity = physics_circle_b->velocity;

                    check_collision = engine_physics_check_circle_circle_collision;
                    get_contact = NULL;

                    collision_cb_a = physics_circle_a->collision_cb;
                    collision_cb_b = physics_circle_b->collision_cb;
                }

                // Only check collision if at atleast one of the involved nodes
                // is dynamic (do not want static nodes generating collisions)
                if(physics_node_a_dynamic == true || physics_node_b_dynamic == true){

                    // To consistently generate the index for `already_collided`
                    // the ID pair needs to be sorted consistently (don't matter
                    // how, just as long as it is consistent)
                    uint32_t pair_index = 0;

                    // Sort so that id_max*a_id + b_id comes out the same
                    // no matter which node is `a` or `b`
                    // // Get the perfect hash code (can be perfect since the range is limited)
                    if(physics_node_a_id > physics_node_b_id){
                        pair_index = engine_physics_get_pair_index(physics_node_b_id, physics_node_a_id);
                    }else{
                        pair_index = engine_physics_get_pair_index(physics_node_a_id, physics_node_b_id);
                    }
                    
                    bool already_collided = engine_bit_collection_get(&collided_physics_nodes, pair_index);

                    // Skip collision check if already collied
                    // before, otherwise set that these have collied
                    // this frame already
                    if(already_collided){
                        physics_link_node_b = physics_link_node_b->next;
                        continue;
                    }else{
                        engine_bit_collection_set(&collided_physics_nodes, pair_index);
                    }

                    float collision_normal_x = 0.0f;
                    float collision_normal_y = 0.0f;
                    float collision_contact_x = 0.0f;
                    float collision_contact_y = 0.0f;
                    float collision_normal_penetration = 0.0f;

                    if(check_collision(physics_node_base_a, physics_node_base_b, &collision_normal_x, &collision_normal_y, &collision_contact_x, &collision_contact_y, &collision_normal_penetration)){
                        // `collision_normal_x` and `collision_normal_y` will point in any direction,
                        // need to discern if the normal axis should be flipped so that the objects
                        // move away from each other: https://stackoverflow.com/a/6244218
                        float a_to_b_direction_x = physics_node_b_position->x - physics_node_a_position->x;
                        float a_to_b_direction_y = physics_node_b_position->y - physics_node_a_position->y;

                        if(engine_math_dot_product(collision_normal_x, collision_normal_y, a_to_b_direction_x, a_to_b_direction_y) >= 0.0f){
                            collision_normal_x = -collision_normal_x;
                            collision_normal_y = -collision_normal_y;
                        }

                        // Resolve collision: https://code.tutsplus.com/how-to-create-a-custom-2d-physics-engine-the-basics-and-impulse-resolution--gamedev-6331t#:~:text=more%20readable%20than%20mathematical%20notation!
                        // If either node is not dynamic, set any velocities to zero no matter what set to
                        if(!physics_node_a_dynamic){
                            physics_node_a_velocity->x = 0.0f;
                            physics_node_a_velocity->y = 0.0f;
                        }

                        if(!physics_node_b_dynamic){
                            physics_node_b_velocity->x = 0.0f;
                            physics_node_b_velocity->y = 0.0f;
                        }

                        float relative_velocity_x = physics_node_b_velocity->x - physics_node_a_velocity->x;
                        float relative_velocity_y = physics_node_b_velocity->y - physics_node_a_velocity->y;

                        float velocity_along_collision_normal = engine_math_dot_product(relative_velocity_x, relative_velocity_y, collision_normal_x, collision_normal_y);

                        // Do not resolve if velocities are separating (this does mean
                        // objects inside each other will not collide until a non separating
                        // velocity is set)
                        if(velocity_along_collision_normal <= 0.0f){
                            physics_link_node_b = physics_link_node_b->next;
                            continue;
                        }

                        if(get_contact != NULL) get_contact(collision_normal_x, collision_normal_y, &collision_contact_x, &collision_contact_y, physics_node_base_a->node, physics_node_base_b->node);

                        // Calculate restitution/bounciness
                        float physics_node_a_bounciness = mp_obj_get_float(mp_load_attr(physics_node_base_a->attr_accessor, MP_QSTR_bounciness));
                        float physics_node_b_bounciness = mp_obj_get_float(mp_load_attr(physics_node_base_b->attr_accessor, MP_QSTR_bounciness));
                        float physics_node_a_mass = mp_obj_get_float(mp_load_attr(physics_node_base_a->attr_accessor, MP_QSTR_mass));
                        float physics_node_b_mass = mp_obj_get_float(mp_load_attr(physics_node_base_b->attr_accessor, MP_QSTR_mass));
                        float bounciness = fminf(physics_node_a_bounciness, physics_node_b_bounciness); // Restitution

                        float physics_node_a_inverse_mass = 0.0f;
                        float physics_node_b_inverse_mass = 0.0f;

                        // Avoid divide by zero for inverse mass calculation
                        if(physics_node_a_mass == 0.0f){
                            physics_node_a_inverse_mass = 0.0f;
                        }else{
                            physics_node_a_inverse_mass = 1.0f / physics_node_a_mass;
                        }

                        if(physics_node_b_mass == 0.0f){
                            physics_node_b_inverse_mass = 0.0f;
                        }else{
                            physics_node_b_inverse_mass = 1.0f / physics_node_b_mass;
                        }

                        float impulse_coefficient_j = -(1 + bounciness) * velocity_along_collision_normal;
                        impulse_coefficient_j /= physics_node_a_inverse_mass + physics_node_b_inverse_mass;

                        float impulse_x = impulse_coefficient_j * collision_normal_x;
                        float impulse_y = impulse_coefficient_j * collision_normal_y;

                        physics_node_a_velocity->x -= physics_node_a_inverse_mass * impulse_x;
                        physics_node_a_velocity->y -= physics_node_a_inverse_mass * impulse_y;

                        physics_node_b_velocity->x += physics_node_b_inverse_mass * impulse_x;
                        physics_node_b_velocity->y += physics_node_b_inverse_mass * impulse_y;

                        // Using the normalized collision normal, offset positions of
                        // both nodes by the amount they were overlapping (in pixels)
                        // when the collision was detected. Split the overlap 50/50
                        //
                        // Depending on which objects are dynamic, move the dynamic bodies by
                        // the penetration amount. Don't want static nodes to be moved by the
                        // penetration amount. 
                        if(physics_node_a_dynamic == true && physics_node_b_dynamic == false){
                            physics_node_a_position->x += collision_normal_x * collision_normal_penetration;
                            physics_node_a_position->y += collision_normal_y * collision_normal_penetration;
                        }else if(physics_node_a_dynamic == false && physics_node_b_dynamic == true){
                            physics_node_b_position->x -= collision_normal_x * collision_normal_penetration;
                            physics_node_b_position->y -= collision_normal_y * collision_normal_penetration;
                        }else if(physics_node_a_dynamic == true && physics_node_b_dynamic == true){
                            physics_node_a_position->x += collision_normal_x * collision_normal_penetration / 2;
                            physics_node_a_position->y += collision_normal_y * collision_normal_penetration / 2;

                            physics_node_b_position->x -= collision_normal_x * collision_normal_penetration / 2;
                            physics_node_b_position->y -= collision_normal_y * collision_normal_penetration / 2;
                        }
                        // If both were not dynamic, that would have been caught
                        // earlier and the collision check would not have happened


                        mp_obj_t collision_contact_data[5];
                        collision_contact_data[0] = mp_obj_new_float(collision_contact_x);
                        collision_contact_data[1] = mp_obj_new_float(collision_contact_y);
                        collision_contact_data[2] = mp_obj_new_float(collision_normal_x);
                        collision_contact_data[3] = mp_obj_new_float(collision_normal_y);

                        mp_obj_t exec[3];

                        // Call A callback
                        collision_contact_data[4] = physics_link_node_b->object;
                        exec[0] = collision_cb_a;
                        exec[1] = physics_node_base_a->attr_accessor;
                        exec[2] = collision_contact_2d_class_new(&collision_contact_2d_class_type, 5, 0, collision_contact_data);
                        mp_call_method_n_kw(1, 0, exec);

                        // Call B callback
                        collision_contact_data[4] = physics_link_node_a->object;
                        exec[0] = collision_cb_b;
                        exec[1] = physics_node_base_b->attr_accessor;
                        exec[2] = collision_contact_2d_class_new(&collision_contact_2d_class_type, 5, 0, collision_contact_data);
                        mp_call_method_n_kw(1, 0, exec);
                    }
                }
            }
            physics_link_node_b = physics_link_node_b->next;
        }
        physics_link_node_a = physics_link_node_a->next;
    }

    // After everything physics related is done, reset the bit array
    // used for tracking which pairs of nodes had already collided
    engine_bit_collection_erase(&collided_physics_nodes);
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