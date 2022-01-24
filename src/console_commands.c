Define_Console_Command(exit) {
    running = false;
}

Define_Console_Command(changemode) {
    if (argument_count != 1) {
        console_printf("provide name of mode\n");
        return;
    } 

    struct console_system_variant modeparam = parameters[0];

    if (!(modeparam.type & CONSOLE_VARIABLE_TYPE_STRING)) {
        console_printf("provide a string\n");
        return;
    }

    char* command_string = modeparam.string;

    if (strcmp(command_string, "edit") == 0) {
        mode = GAME_MODE_EDITOR;
    } else if (strcmp(command_string, "game") == 0) {
        mode = GAME_MODE_PLAYING;
    }
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

void register_console_commands(void) {
    console_system_register_command(&cmd_exit);
    console_system_register_command(&cmd_changemode);
    console_system_register_command(&cmd_editor_save);
    console_system_register_command(&cmd_editor_load);
    console_system_register_command(&cmd_editor_clear);
}
