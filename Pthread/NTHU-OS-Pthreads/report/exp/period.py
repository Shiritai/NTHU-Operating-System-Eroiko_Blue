import numpy as np
import scipy
import matplotlib.pyplot as plt

final= []
firstline=True
linecount=0
aver=[]
data_col=[]
with open("period.log") as f:
    file=f.readlines()
    for line in file:
        
        if firstline:
            firstline=False
            line=line.split("/")
            data_col.append(line[0].strip())
            data_col.append(line[1].strip())
            data_col.append(line[2].strip())
            continue
        temp=[]
        line=line.split("/")
        temp.append(float(eval(line[0].strip())))
        temp.append(float(eval(line[1].strip())))
        temp.append(np.array(eval(line[2].strip())))
        
        aver.append(temp)
        # print(type(temp[0]))
        # print(type(temp[1]))
        # print(type(temp[2]))
        
        if linecount%8==7:
            aver=np.array(aver)
            # print(aver.shape)
            # print(aver)
            cscl=aver[:,2]
            cscl_len=np.zeros(8)
            for a in range(8):
                cscl_len[a]=len(cscl[a])
                # print(cscl[a])
                # print(len(cscl[a]))
            mode=scipy.stats.mode(cscl_len)
            # print(mode[0])
            mask=cscl_len==mode[0]
            # print(aver[mask].shape)
            # result=aver.mean()
            # print(result)
            result=np.mean(aver[mask],axis=0)
            final.append(result)
            aver=[]
            # print(np.mean(aver,axis=0))
        # if linecount==7:
        #     pass
        linecount=linecount+1

data=np.array(final)
with open("period.txt", "w+") as f:
    # data = f.read()
    f.write(str(data_col))
    f.write("\n")
    f.write(str(data))
# np.savetxt("Write_size_data.txt",data,delimiter=',')
# print(data)



x=data[:,0]
y=data[:,1]
z=data[:,2]
print(x)
print(y)
print(type(data_col[0]))
print(data_col[0])
print(type(data_col[1]))
print(data_col[1])


# color=['r','g','b','y','c','m','k','w']

plt.title("period")
plt.xlabel(data_col[0])
plt.ylabel(data_col[1])
plt.plot(x,y)
# plt.show()
plt.savefig("period_vs_time")

# plt.title("period")
# plt.ylabel(data_col[2])
# for t in range(len(z)):
#     plt.plot(z[t],label=x[t],alpha=0.7)
# plt.legend()
# # plt.show()
# plt.savefig("period_cscl")
