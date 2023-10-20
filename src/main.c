#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#define KEY_RETURN 10

typedef struct {
    WINDOW *text;
    WINDOW *text_outline;
    WINDOW *status;
} Window;

typedef struct {
    char *body;
    bool checked;
} Entry;

void handle_sigint();
Window *spawn_window(int height, int width);
void resize_window(Window *win, int height, int width);
char *wget_input(WINDOW *local_win);
Entry *todolist_input_entries(Window *win, int *entries_size);
Entry *todolist_list_entries(Window *win, Entry *entries, int entries_size);

int main() {
    initscr();
    cbreak();
    noecho();
    curs_set(1);
    refresh();

    signal(SIGINT, handle_sigint);

    int height = LINES;
    int width = COLS;
    Window *win = spawn_window(height, width);
    int entries_size;
    Entry *entries;

    wattron(win->text, A_BOLD);
    wprintw(win->text, "Welcome to TODOLIST\n\n");
    wattroff(win->text, A_BOLD);
    wprintw(win->text, "Write your entries in the input bar below, and type\n"
                       "\".e\" to begin checking off entries!\n\n"
                       "Exit using CTRL+C\n");
    wrefresh(win->text);
    wclear(win->text);

    while (true) {
        entries = todolist_input_entries(win, &entries_size);
        entries = todolist_list_entries(win, entries, entries_size);
    }

    endwin();
    return 0;
}

void handle_sigint() {
    endwin();
    exit(0);
}

Window *spawn_window(int height, int width) {
    Window *win;
    win = malloc(sizeof(Window));
    int status_height = 1,
        text_height = height - status_height,
        starty = 0,
        startx = 0;

    win->text_outline = newwin(text_height, width, starty, startx);
    win->text = derwin(win->text_outline, text_height - 2, width - 4, 1, 2);
    win->status = newwin(status_height, width - 4, starty + text_height, 2);
    box(win->text_outline, 0, 0);

    wnoutrefresh(win->text_outline);
    wnoutrefresh(win->text);
    wnoutrefresh(win->status);
    doupdate();

    return win;
}

void resize_window(Window *win, int height, int width) {
    usleep(75000);

    int status_height = 1,
        text_height = height - status_height;

    wclear(win->status);
    wclear(win->text_outline);
    wborder(win->text_outline, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wrefresh(win->text_outline);

    wresize(win->text_outline, text_height, width);
    wresize(win->text, text_height - 2, width - 4);
    wresize(win->status, status_height, width - 4);
    mvwin(win->status, text_height, 2);

    box(win->text_outline, 0, 0);
    wrefresh(win->text_outline);
}

#define INPUT_BUFSIZE 64
char *wget_input(WINDOW *local_win) {
    char *buffer;
    buffer = malloc(INPUT_BUFSIZE);

    wclear(local_win);
    wprintw(local_win, "> ");
    wrefresh(local_win);
    if ((wgetnstr(local_win, buffer, INPUT_BUFSIZE)) == KEY_RESIZE)
        return ".resize";
    wclear(local_win);
    wrefresh(local_win);

    return buffer;
}

#define ENTRIES_BUFSIZE 32
Entry *todolist_input_entries(Window *win, int *entries_size) {
    echo();
    curs_set(1);
    keypad(stdscr, FALSE);
    int entries_bufsize = ENTRIES_BUFSIZE,
        entries_position = 0;
    char *entry;
    Entry *entries = malloc(entries_bufsize * sizeof(Entry));
    if (!entries) {
        endwin();
        printf("Memory allocation error\n");
        exit(1);
    }
    
    do {
        if (!strcmp((entry = wget_input(win->status)), ".resize")) {
            resize_window(win, LINES, COLS);
            wclear(win->text);
            for (int i = 0; i < entries_position; i++)
                wprintw(win->text, "> %s\n", entries[i].body);
            wrefresh(win->text);
            continue;
        }
        entries[entries_position].checked = false;
        entries[entries_position].body = entry;
        wprintw(win->text, "> %s\n", entries[entries_position].body);
        wrefresh(win->text);

        entries_position++;
        if (entries_position >= entries_bufsize) {
            entries_bufsize += ENTRIES_BUFSIZE;
            entries = realloc(entries, entries_bufsize * sizeof(Entry));
        }
    } while (strncmp(entry, ".e", 2));
    *entries_size = entries_position - 1;

    return entries;
}

Entry *todolist_list_entries(Window *win, Entry *entries, int entries_size) {
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    int ch = 0,
        selection = 0;

    wattron(win->status, A_BOLD);
    wprintw(win->status, "q: clear list   e: edit item   CTRL+C: quit program");
    wattroff(win->status, A_BOLD);
    wrefresh(win->status);

    do {
        switch (ch) {
            case 'j':
            case KEY_DOWN:
                selection++;
                if (selection >= entries_size)
                    selection = 0;
                break;
            case 'k':
            case KEY_UP:
                selection--;
                if (selection < 0)
                    selection = entries_size - 1;
                break;
            case 'l':
            case KEY_RETURN:
            case KEY_RIGHT:
                if (!entries[selection].checked)
                    entries[selection].checked = true;
                else
                    entries[selection].checked = false;
                break;
            case 'e':
                echo();
                curs_set(1);
                entries[selection].body = wget_input(win->status);
                noecho();
                curs_set(0);

                wattron(win->status, A_BOLD);
                wprintw(win->status, "q: clear list   e: edit item   CTRL+C: quit program");
                wattroff(win->status, A_BOLD);
                wrefresh(win->status);
                break;
            case KEY_RESIZE:
                resize_window(win, LINES, COLS);

                wclear(win->status);
                wattron(win->status, A_BOLD);
                wprintw(win->status, "q: clear list   e: edit item   CTRL+C: quit program");
                wattroff(win->status, A_BOLD);
                wrefresh(win->status);

                wclear(win->text);
                for (int i = 0; i < entries_size; i++)
                    wprintw(win->text, "> %s\n", entries[i].body);
                wrefresh(win->text);
                break;
        }

        wclear(win->text);
        for (int i = 0; i < entries_size; i++) {
            if (selection == i)
                wattron(win->text, A_STANDOUT);
            if (entries[i].checked)
                wprintw(win->text, "[x] %s\n", entries[i].body);
            else
                wprintw(win->text, "[ ] %s\n", entries[i].body);
            wstandend(win->text);
        }
        wrefresh(win->text);
    } while ((ch = getch()) != 'q');

    wclear(win->text);
    wrefresh(win->text);

    return entries;
}
