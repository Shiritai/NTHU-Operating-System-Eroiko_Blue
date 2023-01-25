import tqdm

tot = input()
width = len(tot)
tot = int(tot)
for i in tqdm.tqdm(range(tot // 10)):
    ls = ['{:0{:}d}'.format(j + 1, width) for j in range(i * 10, (i + 1) * 10)]
    print(" ".join(ls))