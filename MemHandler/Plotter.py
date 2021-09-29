import csv
import numpy as np
import matplotlib.pyplot as plt
def mean(arr):
    tmp = []

    for val in arr:
        if  val != 0:
            tmp.append(val)
    return sum(tmp)/len(tmp)

rows = []
filename = 'profile1.dat'
y1 = []
y2 = []
with open(filename, 'r') as csvfile:
    csvreader = csv.reader(csvfile)
    for row in csvreader:
        rows.append(row)
    print("Lines:",csvreader.line_num)
    myLen = csvreader.line_num-1
    time = 0.05* myLen
    t = np.linspace(0,time,myLen)
    print("Time1:",time)
    for row in rows[:-1]:
        jiff,minor,major,cpu_util = row[0].split(" ")
        # print(jiff,minor,major,cpu_util)
        y1.append(int(minor)+int(major))
        # y2.append(int(major))

    #plt.plot(t,y1)
    gap = max(y1)/10
    yrange = np.arange(0,max(y1),gap)
    #plt.yticks(yrange)
    plt.plot(t,y1)

    # plt.savefig('p1.png')
    # plt.show()

rows = []
filename = 'profile2.dat'
y1 = []
y2 = []
with open(filename, 'r') as csvfile:
    csvreader = csv.reader(csvfile)
    for row in csvreader:
        rows.append(row)
    print("Lines:",csvreader.line_num)
    myLen = csvreader.line_num-1
    time = 0.05* myLen
    t = np.linspace(0,time,myLen)
    print("Time2:",time)
    for row in rows[:-1]:
        jiff,minor,major,cpu_util = row[0].split(" ")
        # print(jiff,minor,major,cpu_util)
        y1.append(int(minor)+int(major))
        # y2.append(int(major))

    #plt.plot(t,y1)
    gap = max(y1)/10
    yrange = np.arange(0,max(y1),gap)
    #plt.yticks(yrange)
    plt.plot(t,y1)
    plt.legend(['Just Random', 'Local & Random'])
    plt.savefig('p2.png')
    # plt.show()
plt.clf()
x = []
y1 = []
for i in range(1,21):
    x.append(i)
    temp =[]
    filename = 'profile3_'+str(i)+'.data'
    rows = []

    with open(filename, 'r') as csvfile:
        csvreader = csv.reader(csvfile)

        for row in csvreader:
            rows.append(row)
        for row in rows[:-1]:
            jiff,minor,major,cpu_util = row[0].split(" ")
            temp.append(int(cpu_util))
        y1.append(int(mean(temp))/int(100))
plt.plot(x,y1)
plt.savefig('mean-multiprog.png')
plt.clf()
x = []
y1 = []
for i in range(1,21):
    x.append(i)
    temp =[]
    filename = 'profile3_'+str(i)+'.data'
    rows = []

    with open(filename, 'r') as csvfile:
        csvreader = csv.reader(csvfile)

        for row in csvreader:
            rows.append(row)
        for row in rows[:-1]:
            jiff,minor,major,cpu_util = row[0].split(" ")
            temp.append(int(cpu_util))
        y1.append(int(max(temp))/int(100))
plt.plot(x,y1)
plt.savefig('max-multiprog.png')


