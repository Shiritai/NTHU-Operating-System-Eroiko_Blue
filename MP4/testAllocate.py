import math

def AllocateAll(fileSize):
    return Allocate(fileSize, 0)

def Allocate(fileSize: int, currentSize: int):
    N_SECTORS = 28

    _fileSize = fileSize
    remainFileSize = fileSize

    ls = [0] * 28
    startBytes = [0] * 28

    i = 0
    while remainFileSize >= 1:
        curBytes = N_SECTORS ** math.floor(
            math.log(remainFileSize, N_SECTORS)) if i + 1 < N_SECTORS else remainFileSize
        if curBytes == _fileSize:
            curBytes //= N_SECTORS
        if remainFileSize == 1:
            curBytes = 1

        startBytes[i] = currentSize + fileSize - remainFileSize

        if curBytes == 1:
            ls[i] = curBytes
        else:
            start_cache = []
            ls[i], start_cache = Allocate(curBytes, currentSize + fileSize - remainFileSize)
            startBytes[i] = startBytes[i], start_cache

        remainFileSize -= curBytes
        i += 1

    return (ls, startBytes)

def print_start(start: list):
    __print_start(start, [])

def __print_start(start: list, indent_list: list):
    front_proto = "".join('|     ' 
        if i == False else '      ' for i in indent_list)
    for i in range(len(start)):
        last_one = i == len(start) - 1
        front = front_proto + ('└' if last_one else '├') + '---'
        if type(start[i]) == int:
            print(front, " [e]: ", start[i], ', ind: ', i, sep='')
        else: 
            print(front_proto + '|')
            print(front, " [t]: ", start[i][0], ', ind: ', i, sep='')
            __print_start(start[i][1], indent_list + [last_one,])

to_print = True
# for fileSize in range(0, 100000):
#     Allocate(fileSize, to_print)

# test(842, True)
ls, startList = AllocateAll(28**3 + 28**2 + 29)
# ls, startList = AllocateAll(1)
print(ls)
# print(startList)
print_start(startList)
print("Passed all testcases :)")