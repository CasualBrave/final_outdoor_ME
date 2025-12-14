---
title: FINAL Group 10 Outdoor REPORT

---

# FINAL Group 10 Outdoor REPORT 

## Overview

### gui視窗

![image](https://hackmd.io/_uploads/Bkzqcb2G-e.png)
- Teleport 1/2/3：讓玩家相機快速移動到預先定義的三個測試點，方便觀察不同地形與物件分布。
- Enable Magic Normal Map ：　啟用後會切換 `Magic Stone` 的法線貼圖，
- G-buffer View ：提供 `G-buffer` 中各個 `Render Target` 的視覺化。
- Depth Mipmap Viz Split：開啟後，`God View` 會改為顯示深度金字塔（Hierarchical Z-Buffer）各層的 `Mipmap`，用於觀察 `Occlusion Culling` 採樣後的深度資訊是否正確。
可以透過 `Level` 來切換不同解析度的深度貼圖，`Depth Gamma` 控制則可調整深度可視化的對比度。
![image](https://hackmd.io/_uploads/H14QhW3zbl.png)
- Occlusion Culling：用來測試視錐裁切後的物件是否被前景遮擋。
`Occlusion Bias`：在比較深度時加入容許誤差，避免精度問題造成物件被錯誤裁切。
`Fixed Mip Override`：強制以指定的深度金字塔 `Level` 做 `occlusion` 。
![image](https://hackmd.io/_uploads/BJewTWhz-e.png)
- Enable Shadows：控制是否啟用場景中的陰影貼圖渲染。


## Phong shading + Basic GPU-driven rendering + Render scene correctly



![image](https://hackmd.io/_uploads/H1R_9Znfbe.png)

![image](https://hackmd.io/_uploads/ByUqR-3zWx.png)

![image](https://hackmd.io/_uploads/SJdRRWhM-x.png)


## Normal mapping

![image](https://hackmd.io/_uploads/SkyglM2f-e.png)

![image](https://hackmd.io/_uploads/ry6xxf2fZe.png)



## Deferred shading

- World space vertex
![image](https://hackmd.io/_uploads/H1821f3fZl.png)
- World space normal
![image](https://hackmd.io/_uploads/ry5SxM2fWe.png)
![image](https://hackmd.io/_uploads/Sy_IeMnG-x.png)
- Diffuse
![image](https://hackmd.io/_uploads/r1OdxfhGbl.png)
- Specular
![image](https://hackmd.io/_uploads/rkOqlM3Gbx.png)
- Ambient (equal to diffuse)
![image](https://hackmd.io/_uploads/ryR2xM2Mbg.png)


## GPU-driven occlusion culling

![image](https://hackmd.io/_uploads/rk6vUMnfbx.png)

### GUI
![image](https://hackmd.io/_uploads/rJhnSXhf-l.png)

BIAS ：調整遮蔽判斷門檻，越大越容易判定為被遮擋。
MIP  ：控制使用哪一層 Mipmap 進行遮擋判斷，層級越高解析度越低、速度越快。


### 不同LEVEL下的裁切與對應MIPMAP
#### LEVEL 0
![image](https://hackmd.io/_uploads/SyZamXhzbg.png)

#### LEVEL 1
![image](https://hackmd.io/_uploads/H1s07Q2z-l.png)

#### LEVEL 2
![image](https://hackmd.io/_uploads/B1rlEQhz-e.png)

#### LEVEL 3
![image](https://hackmd.io/_uploads/B1EMVm2f-g.png)

#### LEVEL 4
![image](https://hackmd.io/_uploads/B1nB4mnGZx.png)

#### LEVEL 5
![image](https://hackmd.io/_uploads/SkewVQ2GZl.png)

#### LEVEL 6
![image](https://hackmd.io/_uploads/H1-cE7hGZg.png)

#### LEVEL 7
![image](https://hackmd.io/_uploads/SyPiEQ3Gbx.png)

#### LEVEL 8
![image](https://hackmd.io/_uploads/HyohE7hfZg.png)

#### LEVEL 9
![image](https://hackmd.io/_uploads/HkhaNQnfZl.png)

## Cascade shadow mapping

### GUI
![image](https://hackmd.io/_uploads/BJEWUX2GWl.png)
ENABLE ：　啟用或關閉 CSM 陰影。
RGB    ：　可切換顯示不同 cascade 的陰影範圍或直接觀察 shadow map 深度。

### 表現
![image](https://hackmd.io/_uploads/BJgtIm3zWg.png)
![image](https://hackmd.io/_uploads/ryWiLQnf-g.png)


