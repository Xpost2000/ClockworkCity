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
    console_printf("Okay! Loaded \"%s\" :)\n", command_string);
}

Define_Console_Command(editor_clear) {
    console_printf("cleared editor stuff!\n");
    editor_clear_all();
}

Define_Console_Command(editor_playtest) {
    console_printf("loading current editor map into game... As if new spawn\n");
    editor_serialize_into_game_memory();
    mode = GAME_MODE_PLAYING;
    player.x = editor_camera.visual_position_x;
    player.y = editor_camera.visual_position_y;
}

/*TODO(jerry): STUPID, allow direct loading from the file!*/
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
    /* editor_load_from_binary_file(command_string); */
    /* editor_serialize_into_game_memory(); */
    mode = GAME_MODE_PLAYING;
}

Define_Console_Command(noclip) {
    console_printf("dirty cheater\n");
    noclip ^= 1;
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
}
