Define_Console_Command(exit) {
    running = false;
}

Define_Console_Command(editor) {
    mode = GAME_MODE_EDITOR;
}

Define_Console_Command(gameplay) {
    mode = GAME_MODE_PLAYING;
}

Define_Console_Command(editor_save) {
    if (argument_count != 1) {
        console_printf("provide file name\n");
        return;
    } 

    struct console_system_variant mode = parameters[0];

    if (!(mode.type & CONSOLE_VARIABLE_TYPE_STRING)) {
        console_printf("filename must be string\n");
        return;
    }

    char* command_string = mode.string;
    editor_output_to_binary_file(command_string);
    console_printf("Okay! Wrote \"%s\" :)\n", command_string);
}

Define_Console_Command(editor_load) {
    if (argument_count != 1) {
        console_printf("provide file name\n");
        return;
    } 

    struct console_system_variant mode = parameters[0];

    if (!(mode.type & CONSOLE_VARIABLE_TYPE_STRING)) {
        console_printf("filename must be string\n");
        return;
    }

    char* command_string = mode.string;
    editor_load_from_binary_file(command_string);
    console_execute_cstr("editor");
    console_printf("Okay! Loaded \"%s\" :)\n", command_string);
}

Define_Console_Command(editor_clear) {
    console_printf("cleared editor stuff!\n");
    editor_clear_all();
}

Define_Console_Command(editor_playtest) {
    struct entity* player = &game_state->persistent_entities[0];

    console_printf("loading current editor map into game... As if new spawn\n");
    editor_serialize_into_game_memory();
    entity_halt_motion(player);
    mode = GAME_MODE_PLAYING;
}

Define_Console_Command(load) {
    if (argument_count != 1) {
        console_printf("provide file name\n");
        return;
    } 

    struct console_system_variant csmode = parameters[0];
    if (!(csmode.type & CONSOLE_VARIABLE_TYPE_STRING)) {
        console_printf("filename must be string\n");
        return;
    }

    char* command_string = csmode.string;
    game_load_level(&game_memory_arena, command_string, 0);
    mode = GAME_MODE_PLAYING;
}

Define_Console_Command(noclip) {
    console_printf("dirty cheater\n");
    noclip ^= 1;
}

Define_Console_Command(use_color) {
    if (argument_count != 1) {
        console_printf("provide color name\n");
        return;
    } 

    struct console_system_variant csmode = parameters[0];
    if (!(csmode.type & CONSOLE_VARIABLE_TYPE_STRING)) {
        console_printf("colorname is a string\n");
        return;
    }

    use_colorscheme(csmode.string);
}

Define_Console_Command(kill) {
    console_printf("I hope it was worth it!");
    struct entity* player = &game_state->persistent_entities[0];
    player->health        = 0; /* We'll animate based off of this state */
}

void register_console_commands(void) {
    console_system_register_command(&cmd_exit);
    console_system_register_command(&cmd_noclip);
    console_system_register_command(&cmd_editor);
    console_system_register_command(&cmd_gameplay);
    console_system_register_command(&cmd_editor_save);
    console_system_register_command(&cmd_editor_load);
    console_system_register_command(&cmd_editor_clear);
    console_system_register_command(&cmd_editor_playtest);
    console_system_register_command(&cmd_load);
    console_system_register_command(&cmd_use_color);
    console_system_register_command(&cmd_kill);
}
