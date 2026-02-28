import sys
if len(sys.argv) > 1:
    with open(sys.argv[1], 'rb') as f:
        data = f.read().replace(b'\r\n', b'\n')
    with open(sys.argv[1], 'wb') as f:
        f.write(data)
