/*
 *  mgbvtd.cpp
 *  vortrac
 *
 *  Created by Xiaowen Tang on 5/28/13.
 *  Copyright 2013 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */
#include <vector>
#include <cmath>
#include <QDebug>

#include <Eigen/Dense>   // added

#include "mgbvtd.h"

using namespace std;

MGBVTD::MGBVTD(float x0, float y0, float hgt, float rmw, GriddedData& cappi)
: m_cappi(cappi),
  m_centerx(x0),
  m_centery(y0),
  m_centerz(hgt),
  m_rmw(rmw)
{
}

float MGBVTD::computeCrossBeamWind(float guessMax, QString& velField, GBVTD* gbvtd, Hvvp* hvvp)
{
    vector<float> comp;
    vector<float> tempComp;
    vector<float>::iterator it, itc;
    vector<float> tempXt;
    vector<float>::iterator itx, itxx;
    float Rt;
    // float Rtr;
    float cc0, cc1, cc2, cc3, cc4, cc5, cc6;
    float curDev = 0;
    float lastDev = 0;
    float minDev = 1000000.0;

    int xindex = int((m_centerx - m_cappi.getX0())/m_cappi.getDeltaX());
    int yindex = int((m_centery - m_cappi.getY0())/m_cappi.getDeltaY());
    int zindex = int((m_centerz - m_cappi.getZ0())/m_cappi.getDeltaZ());

    float tmpBest, tmpXt;

    // loop
    for(int Loop=0; Loop<8; Loop++)
    {
        float x0 = m_cappi.getX0() + xindex*m_cappi.getDeltaX();
        float y0 = m_cappi.getY0() + yindex*m_cappi.getDeltaY();
        float r  = sqrt((m_centerx-x0)*(m_centerx-x0) + (m_centery-y0)*(m_centery-y0));

        // float shift = abs(r-m_rmw)/m_rmw;
        float rmw_temp;
        if(r <= m_rmw)
            rmw_temp = 1.0;
        else if(r <= 2.0*m_rmw)
            rmw_temp = 2.0 - r/m_rmw;
        else
            rmw_temp = 0.0;

        // hvvp
        // Note: use hvvp at a lower level
        int hindex = zindex;
        if(hindex >= 0 && hindex < m_cappi.getNz())
        {
            hvvp->getHvvp_cc(xindex, yindex, hindex, cc0, cc1, cc2, cc3, cc4, cc5, cc6);
        }
        else
        {
            hvvp->getHvvp_cc(xindex, yindex, zindex, cc0, cc1, cc2, cc3, cc4, cc5, cc6);
        }

        float guessvalue;
        int guesscomp;
        // comp, Xt range
        comp.clear();
        for(int k=0; k<=10; k++)
            comp.push_back(guessMax*0.5f + k*guessMax*0.05f);

        // Rt
        // Rtr = r*2.0*0.5/m_cappi.getDeltaX(); // Range to RMSEw Zero?
        Rt = r*2.0f*0.5f/m_cappi.getDeltaX();

        // initial Xt and RMS difference
        float Xmin, Xmax, dx;
        dx = 0.005f;
        // X range from 0 to 1
        Xmin = 0.005f;
        Xmax = 0.995f;

        tempXt.clear();
        for(float Xt=Xmin; Xt<=Xmax; Xt+=dx)
        {
            tempXt.push_back(Xt);
        }

        lastDev = 0;
        minDev = 1000000.0f;

        for(it=comp.begin(), guesscomp=0; it!=comp.end(); it++, guesscomp++)
        {
            // initial error value
            lastDev = 0;

            // calculate the residual error (store into an Eigen vector instead of arma::mat)
            Eigen::VectorXd Delta(tempXt.size());
            Eigen::VectorXd RMS(tempXt.size());

            int j = 0;
            for(itx=tempXt.begin(); itx!=tempXt.end(); ++itx, ++j)
            {
                float hvvp_vm = cc0 - Rt * cc6 / ((*itx) + 1.0f);

                float rmx = rmw_temp * (*it);
                float xt  = (*itx);

                float VecW = gbvtd->getWM(xindex, yindex, zindex);
                // float Ruralpha = gbvtd->getRuralpha(xindex, yindex, zindex);
                // float Rurbeta  = gbvtd->getRurbeta(xindex, yindex, zindex);

                float rms = fabs(VecW - rmx * (1.0f - xt) - hvvp_vm);

                Delta(j) = rms;
            }

            // smoothing (replicates original logic; note: original comparisons below used the unsmoothed sequence)
            const int dim = static_cast<int>(Delta.size());
            if (dim >= 2) {
                RMS(0) = 0.75*Delta(0) + 0.25*Delta(1);
                for(int i=1; i<dim-1; ++i) {
                    RMS(i) = 0.25*Delta(i-1) + 0.5*Delta(i) + 0.25*Delta(i+1);
                }
                RMS(dim-1) = 0.25*Delta(dim-2) + 0.75*Delta(dim-1);
            } else if (dim == 1) {
                RMS(0) = Delta(0);
            }

            // find the min (preserve original behavior: compare the unsmoothed values)
            vector<float> DeltaVec;
            DeltaVec.reserve(dim);
            for (int i=0; i<dim; ++i) DeltaVec.push_back(static_cast<float>(Delta(i)));

            vector<float>::iterator it2;
            tempComp.clear();
            for(itx=tempXt.begin(), it2=DeltaVec.begin(); itx!=tempXt.end(); itx++, it2++)
            {
                if( (*it2) < minDev )
                {
                    minDev = (*it2);
                    guessvalue = (*it);
                    tmpXt = (*itx);
                }
                tempComp.push_back(minDev);
            }
        } // it loop

        // refine the iteration
        if(minDev < curDev || Loop == 0)
        {
            curDev = minDev;

            xindex = int((m_centerx - m_cappi.getX0())/m_cappi.getDeltaX());
            yindex = int((m_centery - m_cappi.getY0())/m_cappi.getDeltaY());
            // zindex = int((m_centerz - m_cappi.getZ0())/m_cappi.getDeltaZ());

            if(xindex > 0 && xindex < m_cappi.getNx()-1)
            {
                float Xmid = tmpXt;
                float Xmin2 = Xmid - 0.1f;
                float Xmax2 = Xmid + 0.1f;

                if(Xmin2 < 0.005f) Xmin2 = 0.005f;
                if(Xmax2 > 0.995f) Xmax2 = 0.995f;

                tempXt.clear();
                for(float Xt=Xmin2; Xt<=Xmax2; Xt+=0.01f)
                    tempXt.push_back(Xt);

                // refine the RMS
                vector<float> DeltaVec;
                DeltaVec.clear();

                for(itx=tempXt.begin(); itx!=tempXt.end(); itx++)
                {
                    float hvvp_vm = cc0 - Rt * cc6 / ((*itx) + 1.0f);
                    float rmx = rmw_temp * guessvalue;
                    float xt  = (*itx);

                    float VecW = gbvtd->getWM(xindex, yindex, zindex);
                    // float Ruralpha = gbvtd->getRuralpha(xindex, yindex, zindex);
                    // float Rurbeta  = gbvtd->getRurbeta(xindex, yindex, zindex);

                    float rms = fabs(VecW - rmx * (1.0f - xt) - hvvp_vm);

                    DeltaVec.push_back(rms);
                }

                // find the min RMS
                float min_rms = 1000000.0f;
                vector<float>::iterator it2;
                for(itx=tempXt.begin(), it2=DeltaVec.begin(); itx!=tempXt.end(); itx++, it2++)
                {
                    if( (*it2) < min_rms )
                    {
                        min_rms = (*it2);
                        tmpXt = (*itx);
                    }
                }
            }

        }

        // new xindex and yindex
        xindex = int((m_centerx - m_cappi.getX0())/m_cappi.getDeltaX());
        yindex = int((m_centery - m_cappi.getY0())/m_cappi.getDeltaY());

    } // loop

    // new X'
    float Xprime = tmpXt;
    // new Wm
    float rmx = gbvtd->getRMX(xindex, yindex, zindex);
    float hvvp_vm = cc0 - Rt * cc6 / (Xprime + 1.0f);
    float VecW = gbvtd->getWM(xindex, yindex, zindex);
    float newWm = VecW - rmx * (1.0f - Xprime) - hvvp_vm;

    return newWm;
}
