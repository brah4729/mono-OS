letters = {
    "M": [
        "  __  __ ",
        " |  \\/  |",
        " | |\\/| |",
        " | |  | |",
        " |_|  |_|"
    ],
    "o": [
        "       ",
        "  ___  ",
        " / _ \\ ",
        "| (_) |",
        " \\___/ "
    ],
    "n": [
        "       ",
        " _ __  ",
        "| '_ \\ ",
        "| | | |",
        "|_| |_|"
    ],
    "O": [
        "  ___  ",
        " / _ \\ ",
        "| | | |",
        "| |_| |",
        " \\___/ "
    ],
    "S": [
        " ____  ",
        "/ ___| ",
        "\\___ \\ ",
        " ___) |",
        "|____/ "
    ]
}
text = ["M", "o", "n", "o", "O", "S"]
for i in range(5):
    line = " ".join([letters[char][i] for char in text])
    # calculate total length to be safe
    line = line.ljust(46, ' ')
    escaped = line.replace("\\", "\\\\").replace("\"", "\\\"")
    print(f'    vga_puts("{escaped}\\n");')
