#include <unistd.h>
#include <linux/uinput.h>
#include <fcntl.h>
#include <string.h>
#include <argp.h>
#include <alsa/asoundlib.h>
#include <stdbool.h>

const char *argp_program_version = "midi-asdf 1.0";
//const char *argp_program_bug_address = "<your@email.address>";
static char doc[] = "MIDI event to /dev/uinput keyboard converter.";
static char args_doc[] = "[MIDI_SOURCE]";
static struct argp_option options[] = {
  { "modifier-keys", 'm', 0, 0, "Enable modifier keys (Ctrl, Shift, Alt) via MIDI keys"},
  { 0 },
};

struct arguments {
  bool enableModifierKeys;
  char *midi_source;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;
  switch (key) {
  case 'm': arguments->enableModifierKeys = true; break;
  case ARGP_KEY_ARG: arguments->midi_source = arg; break;
  default: return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc, 0, 0, 0 };

struct arguments arguments;

static snd_seq_t *seq_handle;
static int in_port;

void emit(int fd, int type, int code, int val)
{
  struct input_event ie;

  ie.type = type;
  ie.code = code;
  ie.value = val;
  ie.time.tv_sec = 0;
  ie.time.tv_usec = 0;

  write(fd, &ie, sizeof(ie));
}

void midi_open(void)
{
  snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_INPUT, 0);

  snd_seq_set_client_name(seq_handle, "midi-asdf");
  in_port = snd_seq_create_simple_port(seq_handle, "MIDI source",
                                       SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
                                       SND_SEQ_PORT_TYPE_APPLICATION);
}

snd_seq_event_t *midi_read(void)
{
  snd_seq_event_t *ev = NULL;
  snd_seq_event_input(seq_handle, &ev);
  return ev;
}

int map_midi_key_to_computer_key(int note)
{
  // First MIDI Key used is B0 (0x23, 35), which is about 2 octaves C3 (aka middle C, 0x3C, 60),
  // mapping to the [Esc] key
  // Last MIDI Key used is G5 (0x5b, 91)
  int offset = -34;
  if (note < 0x23 || note > 0x5b)
    return -1;
  if (!arguments.enableModifierKeys)
    {
      if (note == 0x3f /* LCtrl */
          || note == 0x4c /* LShift */
          || note == 0x58 /* RShift */
          || note == 0x5a /* LAlt */
          )
        return -2;
    }
  return ((int) note) + offset;
}

void midi_process(int fd, snd_seq_event_t *ev)
{
  if ((ev->type == SND_SEQ_EVENT_NOTEON)||(ev->type == SND_SEQ_EVENT_NOTEOFF))
    {
      int computer_key = map_midi_key_to_computer_key(ev->data.note.note);
      if (computer_key >= 0)
        {
          int emit_type = ev->type == SND_SEQ_EVENT_NOTEON ? 1 : 0;
          emit(fd, EV_KEY, computer_key, emit_type);
          emit(fd, EV_SYN, SYN_REPORT, 0);
        }
      else if (computer_key == -1)
        {
          const char *type = (ev->type == SND_SEQ_EVENT_NOTEON) ? "on " : "off";
          printf("[%d] Note %s: %2x vel(%2x)\n", ev->time.tick, type,
                 ev->data.note.note,
                 ev->data.note.velocity);
        }
    }
  else if(ev->type == SND_SEQ_EVENT_CONTROLLER)
    {
      if (ev->data.control.param == 0x43 /* left pedal */)
        {
          emit(fd, EV_KEY, KEY_LEFTALT, ev->data.control.value == 0 ? 0 : 1);
          emit(fd, EV_SYN, SYN_REPORT, 0);
        }
      else if (ev->data.control.param == 0x42 /* middle pedal */)
        {
          emit(fd, EV_KEY, KEY_LEFTCTRL, ev->data.control.value == 0 ? 0 : 1);
          emit(fd, EV_SYN, SYN_REPORT, 0);
        }
      else if (ev->data.control.param == 0x40 /* right pedal */)
        {
          emit(fd, EV_KEY, KEY_LEFTSHIFT, ev->data.control.value == 0 ? 0 : 1);
          emit(fd, EV_SYN, SYN_REPORT, 0);
        }
      else
        {
          printf("[%d] Control:  %2x val(%2x)\n", ev->time.tick,
                 ev->data.control.param,
                 ev->data.control.value);
        }
    }
  /* else */
  /*   printf("[%d] Unknown:  Unhandled Event Received\n", ev->time.tick); */
}

int setup_uinput()
{
  struct uinput_setup usetup;

  int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

  ioctl(fd, UI_SET_EVBIT, EV_KEY);

  // /usr/src/linux-source-5.13.0/linux-source-5.13.0/include/uapi/linux/input-event-codes.h
  ioctl(fd, UI_SET_KEYBIT, KEY_ESC); // 1
  ioctl(fd, UI_SET_KEYBIT, KEY_1); // 2
  ioctl(fd, UI_SET_KEYBIT, KEY_2); // 3
  ioctl(fd, UI_SET_KEYBIT, KEY_3); // 4
  ioctl(fd, UI_SET_KEYBIT, KEY_4); // 5
  ioctl(fd, UI_SET_KEYBIT, KEY_5); // 6
  ioctl(fd, UI_SET_KEYBIT, KEY_6); // 7
  ioctl(fd, UI_SET_KEYBIT, KEY_7); // 8
  ioctl(fd, UI_SET_KEYBIT, KEY_8); // 9
  ioctl(fd, UI_SET_KEYBIT, KEY_9); // 10
  ioctl(fd, UI_SET_KEYBIT, KEY_0); // 11
  ioctl(fd, UI_SET_KEYBIT, KEY_MINUS); // 12
  ioctl(fd, UI_SET_KEYBIT, KEY_EQUAL); // 13
  ioctl(fd, UI_SET_KEYBIT, KEY_BACKSPACE); // 14
  ioctl(fd, UI_SET_KEYBIT, KEY_TAB); // 15
  ioctl(fd, UI_SET_KEYBIT, KEY_Q); // 16
  ioctl(fd, UI_SET_KEYBIT, KEY_W); // 17
  ioctl(fd, UI_SET_KEYBIT, KEY_E); // 18
  ioctl(fd, UI_SET_KEYBIT, KEY_R); // 19
  ioctl(fd, UI_SET_KEYBIT, KEY_T); // 20
  ioctl(fd, UI_SET_KEYBIT, KEY_Y); // 21
  ioctl(fd, UI_SET_KEYBIT, KEY_U); // 22
  ioctl(fd, UI_SET_KEYBIT, KEY_I); // 23
  ioctl(fd, UI_SET_KEYBIT, KEY_O); // 24
  ioctl(fd, UI_SET_KEYBIT, KEY_P); // 25
  ioctl(fd, UI_SET_KEYBIT, KEY_LEFTBRACE); // 26
  ioctl(fd, UI_SET_KEYBIT, KEY_RIGHTBRACE); // 27
  ioctl(fd, UI_SET_KEYBIT, KEY_ENTER); // 28
  ioctl(fd, UI_SET_KEYBIT, KEY_LEFTCTRL); // 29
  ioctl(fd, UI_SET_KEYBIT, KEY_A); // 30
  ioctl(fd, UI_SET_KEYBIT, KEY_S); // 31
  ioctl(fd, UI_SET_KEYBIT, KEY_D); // 32
  ioctl(fd, UI_SET_KEYBIT, KEY_F); // 33
  ioctl(fd, UI_SET_KEYBIT, KEY_G); // 34
  ioctl(fd, UI_SET_KEYBIT, KEY_H); // 35
  ioctl(fd, UI_SET_KEYBIT, KEY_J); // 36
  ioctl(fd, UI_SET_KEYBIT, KEY_K); // 37
  ioctl(fd, UI_SET_KEYBIT, KEY_L); // 38
  ioctl(fd, UI_SET_KEYBIT, KEY_SEMICOLON); // 39
  ioctl(fd, UI_SET_KEYBIT, KEY_APOSTROPHE); // 40
  ioctl(fd, UI_SET_KEYBIT, KEY_GRAVE); // 41
  ioctl(fd, UI_SET_KEYBIT, KEY_LEFTSHIFT); // 42
  ioctl(fd, UI_SET_KEYBIT, KEY_BACKSLASH); // 43
  ioctl(fd, UI_SET_KEYBIT, KEY_Z); // 44
  ioctl(fd, UI_SET_KEYBIT, KEY_X); // 45
  ioctl(fd, UI_SET_KEYBIT, KEY_C); // 46
  ioctl(fd, UI_SET_KEYBIT, KEY_V); // 47
  ioctl(fd, UI_SET_KEYBIT, KEY_B); // 48
  ioctl(fd, UI_SET_KEYBIT, KEY_N); // 49
  ioctl(fd, UI_SET_KEYBIT, KEY_M); // 50
  ioctl(fd, UI_SET_KEYBIT, KEY_COMMA); // 51
  ioctl(fd, UI_SET_KEYBIT, KEY_DOT); // 52
  ioctl(fd, UI_SET_KEYBIT, KEY_SLASH); // 53
  ioctl(fd, UI_SET_KEYBIT, KEY_RIGHTSHIFT); // 54
  ioctl(fd, UI_SET_KEYBIT, KEY_KPASTERISK); // 55
  ioctl(fd, UI_SET_KEYBIT, KEY_LEFTALT); // 56
  ioctl(fd, UI_SET_KEYBIT, KEY_SPACE); // 57
  ioctl(fd, UI_SET_KEYBIT, KEY_CAPSLOCK); // 58
  ioctl(fd, UI_SET_KEYBIT, KEY_F1); // 59
  ioctl(fd, UI_SET_KEYBIT, KEY_F2); // 60
  ioctl(fd, UI_SET_KEYBIT, KEY_F3); // 61
  ioctl(fd, UI_SET_KEYBIT, KEY_F4); // 62
  ioctl(fd, UI_SET_KEYBIT, KEY_F5); // 63
  ioctl(fd, UI_SET_KEYBIT, KEY_F6); // 64
  ioctl(fd, UI_SET_KEYBIT, KEY_F7); // 65
  ioctl(fd, UI_SET_KEYBIT, KEY_F8); // 66
  ioctl(fd, UI_SET_KEYBIT, KEY_F9); // 67
  ioctl(fd, UI_SET_KEYBIT, KEY_F10); // 68

  memset(&usetup, 0, sizeof(usetup));
  usetup.id.bustype = BUS_USB;
  usetup.id.vendor = 0x1234;
  usetup.id.product = 0x5678;
  strcpy(usetup.name, "midi-asdf");

  ioctl(fd, UI_DEV_SETUP, &usetup);
  ioctl(fd, UI_DEV_CREATE);

  return fd;
}

int tear_down_uinput(int fd)
{
  ioctl(fd, UI_DEV_DESTROY);
  close(fd);
}


int main(int argc, char *argv[])
{
  arguments.enableModifierKeys = false;
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  int fd = setup_uinput();

  midi_open();
  while (1)
    midi_process(fd, midi_read());

  tear_down_uinput(fd);

  return 0;
}
