import itertools
with open("short_strings.txt", "w") as f:
    letters = "abcdefghxy"
    digits = "0123456789"
    combos = list(itertools.product(letters, letters, digits))[:1000]
    for a, b, c in combos:
        f.write(f"{a}{b}{c}\n")