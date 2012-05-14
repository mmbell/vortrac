#!/usr/bin/env python
import numpy as np
import matplotlib.pyplot as plt
from xml.dom import minidom
from datetime import datetime

# read simplex result

# read vortrac result
def read_vortex(file_path):
	xmldoc    =minidom.parse(file_path)
	nodes     =xmldoc.getElementsByTagName('record')
	vortex    =[]
	for nn in nodes:
		time    =nn.getElementsByTagName('time')[0].childNodes[0].data
		center  =nn.getElementsByTagName('center')[0].childNodes[0].data
		strength=nn.getElementsByTagName('strength')[0].childNodes[0].data
		time    =datetime.strptime(time,'%Y/%m/%d %H:%M:%S')
		center  =[float(ii) for ii in center.rstrip().split(',')]
		strength=[float(ii) for ii in strength.rstrip().split(',')]
		vortex.append({'time':time,'center':center,'strength':strength})
	return vortex

# read cappi result head-only
def read_cappi_head(file_path):
	f =open(file_path,'r')
	head =[-999]
	for ll in range(1,52):
		line =f.readline()
		lst  =line.split()
		head.extend(lst)
		
	head =np.array(head,dtype='int32')
	return head
	
	
# read cappi result and generate report
def read_cappi(file_path):
	f =open(file_path,'r')
	head =[-999]
	for ll in range(1,52):
		line =f.readline()
		lst  =line.split()
		head.extend(lst)
		
	head =np.array(head,dtype='int32')
	nx,ny,nz,nv =head[162],head[167],head[172],head[175]
	
	data =np.zeros([nx,ny,nv,nz],dtype='float32')
	for kk in range(nz):
		f.readline()
		for yy in range(ny):
			f.readline()
			for vv in range(nv):
				f.readline()
				xline=[]
				while(True):
					ll=f.readline()
					ll=[ll.rstrip()[ii:ii+10] for ii in range(0,len(ll.rstrip()),10)]
					xline.extend(ll)
					if(len(xline)==nx):
						data[:,yy,vv,kk]=np.array(xline,dtype='float32')
						break
	data =np.ma.masked_array(data,mask=(data==-999.0))	
	return head,data
	
	
def dis(Lat1, Lon1, Lat0, Lon0):
	LatRadians = Lat0*np.pi/180.0;
	fac_lat = 111.13209-0.56605*np.cos(2.0*LatRadians)+0.00012*np.cos(4.0*LatRadians)-0.000002*np.cos(6.0*LatRadians);
	fac_lon = 111.41513*np.cos(LatRadians)- 0.09455*np.cos(3.0*LatRadians) + 0.00012*np.cos(5.0*LatRadians);
	return (Lon1-Lon0)*fac_lon,(Lat1-Lat0)*fac_lat
	
	
def main():
	file_path ='/Volumes/Time Machine/ike-khgx/ike_KHGX_2008_voretexlist.xml'
	vortex    = read_vortex(file_path)
	tls       = np.array([ii['time'] for ii in vortex])
	pls       = np.array([ii['strength'][2] for ii in vortex])
	sls       = np.array([ii['strength'][1] for ii in vortex])
	ylim1_min =np.floor(pls.min())
	ylim1_max =np.ceil(pls.max())+10
	ylim1_min -=(ylim1_max-ylim1_min)
	ylim2_min =np.floor(sls.min())-5
	ylim2_max =np.ceil(sls.max())
	ylim2_max +=(ylim2_max-ylim2_min)
	dls=[]
	for tidx in range(len(vortex)):
		file_name =vortex[tidx]['time'].strftime('%Y-%m-%dT%H_%M_%S.asi')
		file_path ='/Volumes/Time Machine/ike-khgx/cappi/'+file_name	
		head =read_cappi_head(file_path)
		reflat =head[33]+head[34]/60.+head[35]/360000.
		reflon =head[36]+head[37]/60.+head[38]/360000.
		vorlat =vortex[tidx]['center'][0]
		vorlon =vortex[tidx]['center'][1]+360.
		vx,vy  =dis(vorlat,vorlon,reflat,reflon)
		dls.append((vx**2+vy**2)**0.5)
	dls=np.array(dls)
	
	for tidx in range(len(vortex)):
		fig=plt.figure(figsize=(11,8.5),dpi=54)
		ax1=plt.subplot2grid((3,4),(0,0),colspan=4)
		ax2=plt.subplot2grid((3,4),(1,0),colspan=2,rowspan=2)
		ax3=plt.subplot2grid((3,4),(1,2),colspan=2,rowspan=2)
		ln1,=ax1.plot(tls,pls,color='purple',lw=2.0,label='pressure')
		ax1.set_ylim(ylim1_min,ylim1_max)
		ax0=ax1.twinx()
		ln2,=ax0.plot(tls,sls,color='darkBlue',lw=2.0,label='rmw')
		ln3,=ax0.plot(tls,dls,'--',color='0.5',lw=2.0,label='distance')
		ax0.set_ylim(1,120)
		ax0.legend([ln1,ln2,ln3],['pressure','rmw','distance'],loc=3)
		

		plt.axvline(tls[tidx],color='red')
		file_name =vortex[tidx]['time'].strftime('%Y-%m-%dT%H_%M_%S.asi')
		print file_name
		vorlat    =vortex[tidx]['center'][0]
		vorlon    =vortex[tidx]['center'][1]+360.
		rmw       =vortex[tidx]['strength'][1]
		file_path ='/Volumes/Time Machine/ike-khgx/cappi/'+file_name
		head,data=read_cappi(file_path)
		reflat =head[33]+head[34]/60.+head[35]/360000.
		reflon =head[36]+head[37]/60.+head[38]/360000.
		xx,yy  =np.meshgrid(range(head[160]/100,head[161]/100),range(head[165]/100,head[166]/100))
		vx,vy  =dis(vorlat,vorlon,reflat,reflon)
		cx,cy  =vx+rmw*np.cos(np.arange(0,2*np.pi,np.pi/100)),vy+rmw*np.sin(np.arange(0,2*np.pi,np.pi/100))
		
	#	plt.figure()
		ax2.pcolor(xx,yy,data[:,:,1,0].transpose())
		ax2.plot(cx,cy,'-',lw=2,color='purple')
		ax2.plot([vx-2,vx+2],[vy,vy],'-',lw=2,color='purple')
		ax2.plot([vx,vx],[vy-2,vy+2],'-',lw=2,color='purple')
		ax2.axis('image')
		ax2.grid(True)
		ax2.text(0.95,0.95,'Vr',transform=ax2.transAxes,ha='center',va='bottom',size=14,weight='bold')
		
	#	plt.figure()
		ax3.pcolor(xx,yy,data[:,:,0,0].transpose())
		ax3.plot(cx,cy,'-',lw=2,color='purple')
		ax3.plot([vx-2,vx+2],[vy,vy],'-',lw=2,color='purple')
		ax3.plot([vx,vx],[vy-2,vy+2],'-',lw=2,color='purple')
		ax3.axis('image')
		ax3.grid(True)
		ax3.text(0.95,0.95,'dBz',transform=ax3.transAxes,ha='center',va='bottom',size=14,weight='bold')
	
	
		fig.text(0.5,0.62,'Ike KHGX',ha='center',va='bottom',size=18,weight='bold')
		plt.tight_layout()
		fig_name =tls[tidx].strftime('%Y%m%d_%H%M%S.png')
		plt.savefig('./'+fig_name,dpi=96)
		plt.show()
		plt.close()
		

if __name__=="__main__":
	main()

