char *keysym_to_string (KeySym keysym, unsigned int modifier);
void cook_keycode (KeyCode keycode, KeySym *keysym, unsigned int *mod);
void get_input (screen_info *s, char *prompt, char *str, int len);
void read_key (KeySym *keysym, unsigned int *mode);
