# Quick note

## 切分方法

```python
N_SECTORS = 29
# if fileSize == 1:
#     return fileSize
# elif fileSize < N_SECTORS:
#     return [1] * fileSize + [0] * (N_SECTORS - fileSize)
if fileSize < N_SECTORS:
    return [1] * fileSize + [0] * (N_SECTORS - fileSize)

_fileSize = fileSize

ls = [0] * 29

i = 0
while i < N_SECTORS - 1 and fileSize > 1:
    cur = N_SECTORS ** math.floor(math.log(fileSize, N_SECTORS))
    if cur == _fileSize:
        cur //= N_SECTORS
    if cur == 1:
        ls[i] = cur
    else:
        ls[i] = run(cur)
    fileSize -= cur
    i += 1

if fileSize != 0:
    ls[i] = run(fileSize)

return ls
```
