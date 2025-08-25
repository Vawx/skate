
static char *forward_to_next_comma(char *ptr, int offset, bool cap_end) {
    for(;;) {if(*ptr++ == ',') {break;}}
    ptr += offset > 0 ? offset : 0;
    if(cap_end) {char *tt = ptr; *tt++; *tt = '\0';}
    return ptr;
}

static int get_int_value_from_following_str(char *temp) {
    char buffer[255] = {0};
    char *bptr = &buffer[0];
    while((u8)*temp >= 48 && (u8)*temp <= 57) {
        *bptr++ = *temp++;
    }
    
    int result = (int)atoi(buffer);
    return result;
}

static char *find_matching_in_buffer(char *buffer, u32 buffer_len, char *phr) {
    u32 len = (u32)strlen(phr);
    LOG_YELL_COND(len < 128, "max length of phrase to find in buffer is 128. no reason. change if needed");
    char bb[128];
    char *bbp = &bb[0];
    
    u32 clen = 0;
    char *temp = buffer;
    while(temp - buffer < buffer_len) {
        if(*temp++ == phr[clen]) {
            ++clen;
            if(clen >= len) {
                return temp;
            }
        } else {
            clen = 0;
        }
    }
    LOG_TELL("failed to find phrase %s in buffer", phr);
    return nullptr;
}

static bool compare_strings_of_length(char *a, char *b, u32 len) {
    int x = 0;
    char *at = a;
    char *bt = b;
    while(x < len) {
        if(*at != *bt) return false;
        ++at; ++bt; ++x;
    }
    return true;
}

static u8 *get_comma_seperated_integers(char *instances_start_ptr, u32 uncompressed_size, const char *ID, const char *src_dir, const u32 alloc_size) {
    LOG_PANIC_COND(alloc_size > 0, "unable to allocate 0 memory.");
    
    char *found = find_matching_in_buffer(instances_start_ptr, uncompressed_size, (char*)ID);
    LOG_YELL_COND(found, "unable to find %s in model file %s", ID, src_dir);
    
    u8 *inst = sokol_skate::mem_alloc(alloc_size);
    u32 *inst_ptr = (u32*)inst;
    char *temp = found;
    char bb[255] = {0};
    char *bbp = &bb[0];
    while(*temp != '{') {
        *bbp = *temp;
        if(*temp++ == ',') {
            *bbp = '\0';
            *inst_ptr++ = atoi(bb);
            bbp = &bb[0];
        } else {
            ++bbp;
        }
    }
    return inst;
}

static bool char_in_ascii_number_range(char v) {
    return v >= 47 && v <= 57;
}

static bool char_could_be_floating_point(char v) {
    return char_in_ascii_number_range(v) || v == '.' || v == '-';
}

static u8 *make_float_array_from_str_buffer(r32 *real_buffer, u8 *str_buffer, u32 *out_size, const s32 expected_size) {
    r32 *rptr = real_buffer;
    char *sstr_buffer = (char*)str_buffer;
    
    char num_buffer[8] = {0};
    memset(num_buffer, 0, 8);
    char *nbp = &num_buffer[0];
    do {
        if(char_could_be_floating_point(*sstr_buffer)) {
            *nbp++ = *sstr_buffer++;
        } else {
            nbp = '\0';
            *rptr++ = atof(num_buffer);
            ++sstr_buffer;
            memset(num_buffer, 0, 8);
            nbp = &num_buffer[0];
        }
    } while((u8*)rptr - (u8*)real_buffer < expected_size);
    LOG_YELL_COND((u8*)rptr - (u8*)real_buffer == expected_size, "float array not of expected range. model may be broken.");
    
    *out_size = (u8*)sstr_buffer - str_buffer;
    
    char *debug_ptr = sstr_buffer;
    while(*debug_ptr != '{' && *debug_ptr != '\0') {*debug_ptr++;}
    u32 debug_count = debug_ptr == sstr_buffer ? 0 : debug_ptr - sstr_buffer;
    LOG_YELL_COND(debug_count == 0, "extra characters in string before next block: %d", debug_count);
    
    return (u8*)real_buffer;
}

template<typename T>
static T *make_array_from_str_buffer(T *type_buffer, u8 *str_buffer, u32 *out_size, const s32 expected_count) {
    T *bptr = type_buffer;
    char *sstr_buffer = (char*)str_buffer;
    
    char num_buffer[8] = {0};
    memset(num_buffer, 0, 8);
    char *nbp = &num_buffer[0];
    do {
        if(char_in_ascii_number_range(*sstr_buffer)) {
            *nbp++ = *sstr_buffer++;
        } else {
            nbp = '\0';
            *bptr++ = (T)atoi(num_buffer);
            ++sstr_buffer;
            memset(num_buffer, 0, 8);
            nbp = &num_buffer[0];
        }
    } while(bptr - type_buffer < expected_count);
    LOG_YELL_COND(bptr - type_buffer == expected_count, "type array not of expected range. model may be broken. expected %d, got %d", expected_count, bptr - type_buffer);
    
    *out_size = (u8*)sstr_buffer - str_buffer;
    
    char *debug_ptr = sstr_buffer;
    while(*debug_ptr != '{' && *debug_ptr != '\0') {*debug_ptr++;}
    u32 debug_count = debug_ptr == sstr_buffer ? 0 : debug_ptr - sstr_buffer;
    LOG_YELL_COND(debug_count == 0, "model loading: extra characters in string before next block: %d", debug_count);
    
    return type_buffer;
}

skate_model_import_t::skate_model_import_t(const skate_directory_t *source) {
    char *str = (char*)strstr((const char*)source->ptr, "model");
    if(!str) { LOG_YELL("trying to import a model that is not .model file: %s", source->ptr); }
    
    skate_file_t file = open_file(source);
    if(file.ptr && file.len > 0) {
        u8 *file_ptr = file.ptr;
        char first_line_buffer[255] = {0};
        char *first_line_buffer_ptr = &first_line_buffer[0];
        
        for(;;) {
            *first_line_buffer_ptr++ = *file_ptr++;
            if(*file_ptr == '\n') {break;}
        }
        
        s32 uncompressed_size = atoi(first_line_buffer);
        if(!uncompressed_size) {
            __debugbreak(); // failed to parse original size.
        }
        
        char *temp = (char*)file.ptr + (first_line_buffer_ptr - &first_line_buffer[0]);
        char *compressed_data = temp + 1; // 1 offset to get past '\n'
        
        Bytef *uncompressed_data = (Bytef*)sokol_skate::mem_alloc(uncompressed_size);
        uLongf new_uncompressed_size = uncompressed_size;
        
        int result = uncompress(uncompressed_data, &new_uncompressed_size, (const Bytef*)compressed_data, file.len - (first_line_buffer_ptr - &first_line_buffer[0]));
        LOG_PANIC_COND(result == Z_OK, "uncompressed failed on file %s with result error: %d", source->ptr, result);
        
        hold_buffer_size = mega(100);
        hold_buffer = sokol_skate::mem_alloc(hold_buffer_size);
        hold_buffer_ptr = hold_buffer;
        
        // NOTE(kyle): update this if adding to beginning of MESH_STATE_ORDER
        mesh_import_state = MESH_STATE_ORDER::VERSION;
        
        memcpy(source_dir.ptr, source->ptr, source->len);
        source_dir.len = source->len;
        
        temp = (char*)uncompressed_data;
        
        next_state: // each state comes back up here with goto after updating state. could be a loop
        switch(mesh_import_state) {
            case MESH_STATE_ORDER::VERSION: {
                char buffer[255] = {0};
                char *ptr = &buffer[0];
                hold_buffer_ptr = (u8*)temp;
                
                while(*hold_buffer_ptr != '\n') {
                    if(!char_could_be_floating_point(*hold_buffer_ptr)) {
                        ++hold_buffer_ptr;
                        continue;
                    }
                    *ptr++ = *hold_buffer_ptr++;
                }
                buffer[ptr - buffer] = '\0';
                
                r32 version = atof(buffer);
                s_assert((s32)1000 * version == (s32)1000 * EXPECTED_VERSION);
                
                temp = (char*)hold_buffer_ptr;
                hold_buffer_ptr = hold_buffer;
                mesh_import_state = MESH_STATE_ORDER::MESH_PATH;
                goto next_state;
            } break;;
            case MESH_STATE_ORDER::MESH_PATH: {
                for(;;) {
                    *hold_buffer_ptr++ = *temp++;
                    if(*temp == '.') {
                        char must_be_fbx[4];
                        char *must_be_fbx_ptr = &must_be_fbx[0];
                        for(int i = 0; i < 4; ++i) {
                            *must_be_fbx_ptr++ =*temp++;
                        }
                        
                        if(!(must_be_fbx[0] == '.' && must_be_fbx[1] == 'f' && must_be_fbx[2] == 'b' && must_be_fbx[3] == 'x')) {
                            LOG_YELL("Incorrect file format or state. Expected that this ends with '.fbx'");
                            goto failed;
                        }
                        import_result.name_len = hold_buffer_ptr - hold_buffer;
                        memcpy(import_result.name, hold_buffer, hold_buffer_ptr - hold_buffer);
                        import_result.name[import_result.name_len] = 0;
                        mesh_import_state = MESH_STATE_ORDER::NUM_PARTS;
                        break;
                    }
                }
                goto next_state;
            } break;
            case MESH_STATE_ORDER::NUM_PARTS: {
                hold_buffer_ptr = hold_buffer;
                temp = forward_to_next_comma(temp, 0, false);
                import_result.num_parts = get_int_value_from_following_str(temp);
                mesh_import_state = MESH_STATE_ORDER::NUM_INSTANCES;
                goto next_state;
            } break;
            case MESH_STATE_ORDER::NUM_INSTANCES: {
                hold_buffer_ptr = hold_buffer;
                temp = forward_to_next_comma(temp, 0, false);
                import_result.num_instances = get_int_value_from_following_str(temp);
                mesh_import_state = MESH_STATE_ORDER::SKINNED;
                goto next_state;
            } break;
            case MESH_STATE_ORDER::SKINNED: {
                hold_buffer_ptr = hold_buffer;
                temp = forward_to_next_comma(temp, 0, false);
                import_result.skinned = get_int_value_from_following_str(temp);
                LOG_YELL_COND(import_result.skinned == 0 || import_result.skinned == 1, "skinned result should be 0 or 1 to tell if the model has skinning or not. because this is out of range, reverting back to 0. %s", source->ptr);
                mesh_import_state = MESH_STATE_ORDER::NUM_BLEND_SHAPES;
                goto next_state;
            } break;
            case MESH_STATE_ORDER::NUM_BLEND_SHAPES: {
                hold_buffer_ptr = hold_buffer;
                temp = forward_to_next_comma(temp, 0, false);
                import_result.num_blend_shapes = get_int_value_from_following_str(temp);
                mesh_import_state = MESH_STATE_ORDER::NUM_BONES;
                goto next_state;
            } break;
            case MESH_STATE_ORDER::NUM_BONES: {
                hold_buffer_ptr = hold_buffer;
                temp = forward_to_next_comma(temp, 0, false);
                import_result.num_bones = get_int_value_from_following_str(temp);
                if(import_result.num_bones == 0 && import_result.skinned == 1) {
                    LOG_TELL("model %s has no bones but the model is skinned");
                    goto failed;
                }
                mesh_import_state = MESH_STATE_ORDER::INSTANCES;
                goto next_state;
            } break;
            case MESH_STATE_ORDER::INSTANCES: {
                import_result.instances_size = import_result.num_instances * sizeof(u32);
                import_result.instances_ptr = get_comma_seperated_integers((char*)uncompressed_data, uncompressed_size, INSTANCES_PTR_ID, (char*)source->ptr, import_result.instances_size);
                mesh_import_state = MESH_STATE_ORDER::BONE_INDICES;
                goto next_state;
            } break;
            case MESH_STATE_ORDER::BONE_INDICES: {
                import_result.bone_indices_size = import_result.num_bones * sizeof(u32);
                if(import_result.bone_indices_size != 0) {
                    import_result.bone_indices_ptr = get_comma_seperated_integers((char*)uncompressed_data, uncompressed_size, BONE_INDICES_PTR_ID, (char*)source->ptr, import_result.bone_indices_size);
                }
                mesh_import_state = MESH_STATE_ORDER::BONE_MATRICES;
                goto next_state;
            } break;
            case MESH_STATE_ORDER::BONE_MATRICES: {
                hold_buffer_ptr = hold_buffer;
                char *bone_matrices_start_ptr = (char*)uncompressed_data;
                char *found = find_matching_in_buffer(bone_matrices_start_ptr, uncompressed_size, (char*)BONE_MATRICES_PTR_ID);
                LOG_YELL_COND(found, "unable to find %s in model file %s", BONE_MATRICES_PTR_ID, source->ptr);
                
                import_result.bone_matrices_size = (sizeof(r32) * 16) * import_result.num_bones;
                import_result.bone_matrices_ptr = sokol_skate::mem_alloc(import_result.bone_matrices_size);
                r32 *result_ptr = (r32*)import_result.bone_matrices_ptr;
                
                char *temp_bmat = found;
                while(*temp_bmat++ != ']') {}
                
                u32 len = temp_bmat - found;
                
                hold_buffer_ptr = hold_buffer;
                temp_bmat = found + 1;
                
                while(temp_bmat - found < len) {
                    if(*temp_bmat == ',') {
                        *temp_bmat++;
                        *hold_buffer_ptr = '\n';
                        *result_ptr++ = atof((char*)hold_buffer);
                        hold_buffer_ptr = hold_buffer;
                        if(result_ptr - (r32*)import_result.bone_matrices_ptr >= 16 * import_result.num_bones) {
                            int a = import_result.num_bones * sizeof(mat4);
                            int b = (u8*)result_ptr - import_result.bone_matrices_ptr;
                            if(a != b) {
                                LOG_YELL("done with bone matrices. expected %d got %d in size", a, b);
                            } else {
                                LOG_TELL("done with bone matrices. expected %d got %d in size", a, b);
                            }
                            break;
                        } 
                        continue;
                    }
                    *hold_buffer_ptr++ = *temp_bmat++;
                }
                
                mesh_import_state = MESH_STATE_ORDER::BLEND_SHAPES;
                goto next_state;
            } break;
            case MESH_STATE_ORDER::BLEND_SHAPES: {
                hold_buffer_ptr = hold_buffer;
                char *blend_shapes_start_ptr = (char*)uncompressed_data;
                char *found = find_matching_in_buffer(blend_shapes_start_ptr, uncompressed_size, (char*)BLEND_SHAPES_PTR_ID);
                LOG_YELL_COND(found, "unable to find %s in model file %s", BLEND_SHAPES_PTR_ID, source->ptr);
                
                if(char_in_ascii_number_range(*found)) {
                    // process...
                    // i dont actually know what this data looks like yet aug 13, 2025
                }
                
                mesh_import_state = MESH_STATE_ORDER::BLEND_SHAPE_IMAGE;
                goto next_state;
            } break;
            case MESH_STATE_ORDER::BLEND_SHAPE_IMAGE: {
                hold_buffer_ptr = hold_buffer;
                char *blend_shape_image_ptr = (char*)uncompressed_data;
                char *found = find_matching_in_buffer(blend_shape_image_ptr, uncompressed_size, (char*)BLEND_SHAPE_IMAGE_PTR_ID);
                LOG_YELL_COND(found, "unable to find %s in model file %s", BLEND_SHAPE_IMAGE_PTR_ID, source->ptr);
                
                char *temp_bsi = found;
                while(*temp_bsi == ':') { ++temp_bsi; }
                
                import_result.blend_shape_image.type = UINT16_MAX;
                import_result.blend_shape_image.width = UINT32_MAX;
                import_result.blend_shape_image.height = UINT32_MAX;
                import_result.blend_shape_image.num_slices = UINT32_MAX;
                import_result.blend_shape_image.pixel_format = UINT32_MAX;
                import_result.blend_shape_image.ptr_size = UINT64_MAX;
                import_result.blend_shape_image.ptr = nullptr;
                
                int order_count = 0;
                
                for(;;) {
                    switch(order_count) {
                        case BLEND_SHAPE_ORDER::TYPE: {
                            import_result.blend_shape_image.type = atoi(temp_bsi);
                        } break;
                        case BLEND_SHAPE_ORDER::WIDTH: {
                            import_result.blend_shape_image.width = atoi(temp_bsi);
                        } break;
                        case BLEND_SHAPE_ORDER::HEIGHT: {
                            import_result.blend_shape_image.height = atoi(temp_bsi);
                        } break;
                        case BLEND_SHAPE_ORDER::NUM_SLICES: {
                            import_result.blend_shape_image.num_slices = atoi(temp_bsi);
                        } break;
                        case BLEND_SHAPE_ORDER::PIXEL_FORMAT: {
                            import_result.blend_shape_image.pixel_format = atoi(temp_bsi);
                        } break;
                        case BLEND_SHAPE_ORDER::PTR_SIZE: {
                            import_result.blend_shape_image.ptr_size = atoi(temp_bsi);
                        } break;
                        case BLEND_SHAPE_ORDER::PTR: {
                            if(*temp_bsi == ' ' || *temp_bsi == '[') {
                                import_result.blend_shape_image.ptr = nullptr;
                            } else {
                                import_result.blend_shape_image.ptr = sokol_skate::mem_alloc(import_result.blend_shape_image.ptr_size);
                                u8 *sip = import_result.blend_shape_image.ptr;
                                while(*temp_bsi != ' ' && *temp_bsi != '[') {
                                    *sip++ = *temp_bsi++;
                                }
                            }
                        } break; 
                    }
                    while(*temp_bsi++ != ',') {}
                    if(++order_count == BLEND_SHAPE_ORDER::COUNT) {break;}
                }
                LOG_PANIC_COND(order_count == (int)BLEND_SHAPE_ORDER::COUNT, "invalid blend shape image data in model import %s", source->ptr);
            } break;
            
            
            // IF MESH_STATE_ORDER IS EXPANDED THERE NEEDS TO BE A STATE SET AND GOTO "next_state" ADDED HERE.
        }
        
        import_result.parts = (skate_model_import_part_t*)sokol_skate::mem_alloc(sizeof(skate_model_import_part_t) * import_result.num_parts);
        u32 mesh_part_idx = 0;
        char *last_mesh_found_point = nullptr;
        u32 last_mesh_found_size = uncompressed_size;
        
        // that was the mesh header, now for each mesh piece
        make_mesh:
        
        temp = last_mesh_found_point != nullptr ? last_mesh_found_point :(char*)uncompressed_data;
        hold_buffer_ptr = hold_buffer;
        
        char id_buffer[255] = {0};
        char *ibp = &id_buffer[0];
        ibp += sprintf(ibp, "%s%d", PART_ID, mesh_part_idx);
        *ibp++ = '\0';
        
        char *found = find_matching_in_buffer(temp, last_mesh_found_size, id_buffer);
        LOG_PANIC_COND(found, "unable to find part identifier %s in model file %s", PART_ID, source->ptr);
        
        skate_model_import_part_t *part = &import_result.parts[mesh_part_idx];
        last_mesh_found_point = found;
        
        int part_ele_idx = 0;
        for(;;) {
            switch(part_ele_idx) {
                case MESH_PART_ORDER::ID: { // id
                    part->id = atoi(last_mesh_found_point);
                } break;
                case MESH_PART_ORDER::MATERIAL_IDX: { // mat index
                    part->mat_idx = atoi(last_mesh_found_point);
                } break;
                case MESH_PART_ORDER::NUM_INDICES: { // indice_count
                    part->indices_count = atoi(last_mesh_found_point);
                } break;
                case MESH_PART_ORDER::VERTEX_BUFFER_SIZE: { // vert_size
                    part->vertices_size = atoi(last_mesh_found_point);
                } break;
                case MESH_PART_ORDER::SKIN_BUFFER_SIZE: { //skin_size
                    part->skin_size = atoi(last_mesh_found_point);
                } break;
            }
            while(*last_mesh_found_point++ != ',') {}
            if(++part_ele_idx == MESH_PART_ORDER::COUNT) {break;}
        }
        
        char *lmffpp_saved = last_mesh_found_point;
        
        // AABB_MIN
        temp = (char*)++last_mesh_found_point;
        last_mesh_found_point = find_matching_in_buffer(temp, last_mesh_found_size, PART_AABB_MIN_ID);
        LOG_PANIC_COND(last_mesh_found_point, "unable to find part identifier %s in model file %s", PART_AABB_MIN_ID, source->ptr);
        
        vec3 rr;
        for(int i = 0; i < sizeof(vec3) / sizeof(r32); ++i) {
            while(*last_mesh_found_point == ',' || *last_mesh_found_point == ':') {++last_mesh_found_point;}
            rr[i] = atof(last_mesh_found_point);
            while(*last_mesh_found_point != ',') {++last_mesh_found_point;}
        }
        glm_vec3_copy(rr, part->aabb_min);
        
        // AABB_MAX
        temp = (char*)lmffpp_saved + 1;
        last_mesh_found_point = find_matching_in_buffer(temp, last_mesh_found_size, PART_AABB_MAX_ID);
        LOG_PANIC_COND(last_mesh_found_point, "unable to find part identifier %s in model file %s", PART_AABB_MAX_ID, source->ptr);
        
        glm_vec3_zero(rr);
        for(int i = 0; i < sizeof(vec3) / sizeof(r32); ++i) {
            while(*last_mesh_found_point == ',' || *last_mesh_found_point == ':') {++last_mesh_found_point;}
            rr[i] = atof(last_mesh_found_point);
            while(*last_mesh_found_point != ',') {++last_mesh_found_point;}
        }
        glm_vec3_copy(rr, part->aabb_max);
        
        last_mesh_found_point = lmffpp_saved;
        
        // we have our mesh part static data, now to pull the pointer data from the file
        temp = (char*)++last_mesh_found_point;
        last_mesh_found_point = find_matching_in_buffer(temp, last_mesh_found_size, PART_INDICES_PTR_ID);
        LOG_PANIC_COND(last_mesh_found_point, "unable to find part identifier %s in model file %s", PART_INDICES_PTR_ID, source->ptr);
        
        // start with indices
        int expected_indice_count = part->indices_count;
        part->indices_ptr = sokol_skate::mem_alloc(expected_indice_count * sizeof(u32));
        
        u32 out_string_length = 0;
        part->indices_ptr =
        (u8*)make_array_from_str_buffer<u16>((u16*)part->indices_ptr, (u8*)last_mesh_found_point, &out_string_length, expected_indice_count);
        last_mesh_found_point += out_string_length;
        
        // do the same thing for vertices
        int vert_id_len = (int)strlen(PART_VERTICES_PTR_ID);
        
        // we expect that based on the file format, the vertices ptr id will be here. stop if not.
        bool comp = compare_strings_of_length(last_mesh_found_point, PART_VERTICES_PTR_ID, vert_id_len);
        LOG_PANIC_COND(comp, "unable to find part identifier %s in model file %s", PART_VERTICES_PTR_ID, source->ptr);
        last_mesh_found_point += vert_id_len;
        
        // move found ptr forward so it starts right at when the vertices ptr data does
        int expected_vertice_size = part->vertices_size;
        part->vertices_ptr = sokol_skate::mem_alloc(expected_vertice_size);
        part->vertices_ptr = make_float_array_from_str_buffer((r32*)part->vertices_ptr, (u8*)last_mesh_found_point, &out_string_length, expected_vertice_size);
        last_mesh_found_point += out_string_length;
        
        // and once more for skin_vertices
        int skin_id_len = (int)strlen(PART_SKINNED_VERTEX_PTR_ID);
        
        // we expect that based on the file format, the vertices ptr id will be here. stop if not.
        comp = compare_strings_of_length(last_mesh_found_point, PART_SKINNED_VERTEX_PTR_ID, skin_id_len);
        LOG_PANIC_COND(comp, "unable to find part identifier %s in model file %s", PART_SKINNED_VERTEX_PTR_ID, source->ptr);
        last_mesh_found_point+= skin_id_len;
        
        int expected_skin_size = part->skin_size;
        if(expected_skin_size) {
            part->skin_ptr = sokol_skate::mem_alloc(expected_skin_size);
            out_string_length = 0;
            part->skin_ptr = make_array_from_str_buffer<u8>(part->skin_ptr, (u8*)last_mesh_found_point, &out_string_length, expected_skin_size);
            last_mesh_found_point += out_string_length;
        }
        
        // expect EOF or next mesh
        ++last_mesh_found_point;
        const char last_char = *last_mesh_found_point;
        if(last_char == '\0') {
            LOG_TELL("model %s loading completed successfully", source->ptr);
            return;
        } else {
            if(++mesh_part_idx < import_result.num_parts) {
                last_mesh_found_size = uncompressed_size - (last_mesh_found_point - (char*)uncompressed_data);
                goto make_mesh;
            }
        }
    }
    
    failed:
    LOG_YELL("failed to load file %s", source->ptr);
    return;
}

skate_model_import_t *skate_import_t::get_or_load_model(const skate_directory_t *dir) {
    for(int i = 0; i < model_import_buffer.count(); ++i) {
        skate_model_import_t *import = model_import_buffer.as_idx<skate_model_import_t>(i * sizeof(model_import_buffer.type_size));
        if(strcmp((char*)import->source_dir.ptr, (char*)dir->ptr) == 0) {
            return import;
        }
    }
    
    if(model_import_buffer.count() * model_import_buffer.type_size >= model_import_buffer.size) {
        LOG_PANIC("out of mesh buffer space to import to!");
        return nullptr;
    }
    
    skate_model_import_t result = skate_model_import_t(dir);
    push_buffer(&model_import_buffer, (u8*)&result, sizeof(skate_model_import_t));
    return model_import_buffer.as_idx<skate_model_import_t>(model_import_buffer.count() - 1);
}