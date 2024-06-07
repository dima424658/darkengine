#pragma once

class cD6OverlayHandler;

class cD6OvelayType {
    cD6OvelayType(const class cD6OvelayType &);
    cD6OvelayType();
    virtual ~cD6OvelayType();

    void TurnOnOff(bool bOn);
    virtual void DrawOverlay();

    unsigned long m_dwID;
    cD6OverlayHandler * m_pcParent;
    cD6OvelayType * m_pcNext;
    cD6OvelayType * m_pcPrevious;
    int m_bVisible;
    int m_bTurnedOff;
};