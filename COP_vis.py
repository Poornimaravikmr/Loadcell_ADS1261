import serial
import threading
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from matplotlib.patches import Rectangle
from matplotlib.gridspec import GridSpec

PORT="COM12"
BAUD=115200

LENGTH=65.8
WIDTH=30.0
HALF_L=LENGTH/2
HALF_W=WIDTH/2

mode=input("""
==========================
 Force Plate Visualizer
==========================
1. Board 1
2. Both Boards

Select option: """).strip()

ser=serial.Serial(PORT,BAUD,timeout=1)
ser.reset_input_buffer()

lock=threading.Lock()
stop_flag=False

class Board:
    def __init__(self,ax,title,color="red"):
        self.ax=ax
        self.copx=0
        self.copy=0
        self.weight=0
        self.forces=[0,0,0,0]
        ax.set_facecolor("black")

        ax.set_xlim(-HALF_W-5,HALF_W+5)
        ax.set_ylim(-HALF_L-5,HALF_L+5)
        ax.set_aspect("equal")
        ax.set_xticks([])
        ax.set_yticks([])
        ax.tick_params(colors="white")
        for s in ax.spines.values():
            s.set_visible(False)
        ax.set_title(title, fontsize=14, color="white", weight="bold")

        ax.add_patch(Rectangle((-HALF_W,-HALF_L),WIDTH,LENGTH,
                               fill=False,lw=3,ec="white"))
        ax.plot([-HALF_W,HALF_W],[0,0],"w--",lw=1)
        ax.plot([0,0],[-HALF_L,HALF_L],"w--",lw=1)
        ax.scatter([-HALF_W,HALF_W,-HALF_W,HALF_W],
                   [HALF_L,HALF_L,-HALF_L,-HALF_L],
                   c="white",s=25)
        self.dot,=ax.plot([],[],"o",ms=14,color=color,animated=True)

    def update(self):
        self.dot.set_data([self.copx],[self.copy])

def serial_worker():
    global b1,b2
    while not stop_flag:
        raw=ser.readline()
        if not raw:
            continue

        # if more has already piled up behind this line, skip straight to the newest
        if ser.in_waiting:
            backlog=ser.read(ser.in_waiting)
            chunks=(raw+backlog).split(b"\n")
            # last element may be a partial line with no trailing \n yet; drop it
            if not (raw+backlog).endswith(b"\n"):
                chunks=chunks[:-1]
            chunks=[c for c in chunks if c.strip()]
            if chunks:
                raw=chunks[-1]

        line=raw.decode(errors="ignore").strip().replace(","," ")
        if not line:
            continue
        try:
            v=[float(x) for x in line.split()]
        except ValueError:
            continue

        with lock:
            if mode=="2":
                if len(v)>=14:
                    b1.copx,b1.copy,b1.weight=v[:3]
                    b2.copx,b2.copy,b2.weight=v[3:6]
                    b1.forces=v[6:10]
                    b2.forces=v[10:14]
            else:
                if len(v)>=7:
                    b1.copx,b1.copy,b1.weight=v[:3]
                    b1.forces=v[3:7]

if mode=="2":
    fig = plt.figure(figsize=(13,8), facecolor="black")
    gs=GridSpec(1,3,width_ratios=[1,1,0.65],wspace=0.15)

    ax1=fig.add_subplot(gs[0,0])
    ax2=fig.add_subplot(gs[0,1])
    info=fig.add_subplot(gs[0,2])
    info.set_facecolor("black")
    info.axis("off")

    b1=Board(ax1,"Board 1","red")
    b2=Board(ax2,"Board 2","blue")

    t = info.text(
    0,
    1,
    "",
    va="top",
    fontsize=12,
    family="monospace",
    color="white",
    animated=True,
)

    def init():
        b1.dot.set_data([],[])
        b2.dot.set_data([],[])
        t.set_text("")
        return b1.dot,b2.dot,t

    def update(frame):
        with lock:
            c1x,c1y,w1,f1 = b1.copx,b1.copy,b1.weight,list(b1.forces)
            c2x,c2y,w2,f2 = b2.copx,b2.copy,b2.weight,list(b2.forces)
        b1.copx,b1.copy=c1x,c1y
        b2.copx,b2.copy=c2x,c2y
        b1.update(); b2.update()
        t.set_text(f"""Board 1

COP X : {c1x:6.2f}
COP Y : {c1y:6.2f}

Weight : {w1:6.2f}

F1 : {f1[0]:6.2f}      F3 : {f1[2]:6.2f}
F2 : {f1[1]:6.2f}      F4 : {f1[3]:6.2f}


Board 2

COP X : {c2x:6.2f}
COP Y : {c2y:6.2f}

Weight : {w2:6.2f}

F5 : {f2[0]:6.2f}      F7 : {f2[2]:6.2f}
F6 : {f2[1]:6.2f}      F8 : {f2[3]:6.2f}
""")
        return b1.dot,b2.dot,t

else:
    fig = plt.figure(figsize=(10,8), facecolor="black")
    gs=GridSpec(1,2,width_ratios=[1,0.5],wspace=0.15)
    ax=fig.add_subplot(gs[0,0])
    info=fig.add_subplot(gs[0,1]); info.axis("off")
    info.set_facecolor("black")
    b1=Board(ax,"Board 1","red")
    t = info.text(0,1,"",va="top",fontsize=12,family="monospace",color="white",animated=True)

    def init():
        b1.dot.set_data([],[])
        t.set_text("")
        return b1.dot,t

    def update(frame):
        with lock:
            cx,cy,w,f = b1.copx,b1.copy,b1.weight,list(b1.forces)
        b1.copx,b1.copy=cx,cy
        b1.update()
        t.set_text(f"""Board 1

COP X : {cx:6.2f}
COP Y : {cy:6.2f}

Weight : {w:6.2f}

F1 : {f[0]:6.2f}
F2 : {f[1]:6.2f}
F3 : {f[2]:6.2f}
F4 : {f[3]:6.2f}
""")
        return b1.dot,t

worker=threading.Thread(target=serial_worker,daemon=True)
worker.start()

ani=FuncAnimation(fig,update,init_func=init,interval=20,blit=True)
plt.show()

stop_flag=True