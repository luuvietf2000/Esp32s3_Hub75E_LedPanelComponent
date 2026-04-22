import string

# Tập ký tự: a-z A-Z 0-9
chars = string.ascii_letters + string.digits

with open("test.imgRaw", "wb") as f:
    for i in range(4106):
        c = chars[i % len(chars)]
        f.write(c.encode())