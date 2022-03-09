/* TODO(jerry): ! */
local void persistent_changes_add_change(struct persistent_changes* changes, char* level_name, struct persistent_change change) {
    struct persistent_change_list* target_list = NULL;
    console_printf("Updated: %s\n", level_name);

    if (level_name == NULL) {
        /* global application */
        target_list = &changes->lists[0];
    } else {
        for (unsigned index = 0; index < changes->list_count; ++index) {
            if (strncmp(changes->lists[index+1].level_name, level_name, FILENAME_MAX_LENGTH) == 0) {
                target_list = &changes->lists[index+1];
                break;
            }
        }
    }

    if (!target_list) {
        target_list = &changes->lists[1 + changes->list_count++];
        strncpy(target_list->level_name, level_name, FILENAME_MAX_LENGTH);
    }

    {
        for (unsigned index = 0; index < MAX_PERSISTENT_CHANGE_LIST_CHANGES; ++index) {
            struct persistent_change* target_change = &target_list->changes[index];
            if (target_change->type == PERSISTENT_CHANGE_NONE) {
                *target_change = change;
                break;
            }
        }
    }
}

local uint32_t persistent_changes_find_change_list_by_name(struct persistent_changes* changes, char* name) {
    if (!name) return 0;

    for (unsigned index = 0; index < changes->list_count; ++index) {
        struct persistent_change_list* change_list = &changes->lists[index+1];

        if (strncmp(change_list->level_name, name, FILENAME_MAX_LENGTH) == 0) {
            return index;
        }
    }

    return -1;
}

/* changes are applied after level load */
local void persistent_changes_apply_changes(struct persistent_changes* changes, struct game_state* state, uint32_t id) {
    /* does not do checking */
    if (id == (uint32_t)-1) return;
    
    struct persistent_change_list* change_list = &changes->lists[id];

    if (id != 0) {
        if (!(strncmp(change_list->level_name, state->current_level_filename, FILENAME_MAX_LENGTH) == 0)) {
            /* do not apply if level names do not match */
            return;
        }
    }

    struct tilemap* level = game_state->loaded_level;
    for (unsigned index = 0; index < MAX_PERSISTENT_CHANGE_LIST_CHANGES; ++index) {
        struct persistent_change* change = &change_list->changes[index];

        switch (change->type) {
            case PERSISTENT_CHANGE_ADD_PLAYER_MOVEMENT_FLAG: {
                struct persistent_change_add_player_movement_flag data = change->add_player_movement_flag;
                struct entity* player = &game_state->persistent_entities[0];
                player->movement_flags |= data.flag;
            } break;
            case PERSISTENT_CHANGE_SOUL_ANCHOR_ACTIVATED: {
                struct persistent_change_soul_anchor_activated data = change->soul_anchor_activated;
                struct soul_anchor* target = level->soul_anchors + data.soul_anchor_index;
                target->unlocked = true;
            } break;
        }
    }
}

local void persistent_changes_serialize(struct persistent_changes* changes, struct binary_serializer* serializer) {
    serialize_u16(serializer, &changes->list_count);
    
    /* Since the persistent_change_list doesn't keep a count (for some reason), we cannot compress the change list
     any further, and I'm not in the mood to change it. So we serialize only change lists that we have. Expands to
     probably 3MB+? Not too happy about that but whatever. */
    Serialize_Structure(serializer, changes->lists[0]);
    for (unsigned index = 0; index < changes->list_count; ++index) {
        Serialize_Structure(serializer, changes->lists[1+index]);
    }
}
