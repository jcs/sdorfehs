#ifndef _RATPOISON_EDITOR_H
#define _RATPOISON_EDITOR_H 1

typedef enum edit_status edit_status;

enum
edit_status
{
  EDIT_INSERT,
  EDIT_DELETE,
  EDIT_MOVE,
  EDIT_COMPLETE,
  EDIT_ABORT,
  EDIT_DONE,
  EDIT_NO_OP
};

/* Input line functions */
rp_input_line *input_line_new (char *prompt, char *preinput, completion_fn fn);
void input_line_free (rp_input_line *line);

edit_status execute_edit_action (rp_input_line *line, KeySym ch, unsigned int modifier, char *keysym_buf);

#endif /* ! _RATPOISON_EDITOR_H */
