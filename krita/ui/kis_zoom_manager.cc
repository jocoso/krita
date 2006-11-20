/*
 *  Copyright (C) 2006 Boudewijn Rempt <boud@valdyas.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#include "kis_zoom_manager.h"

#include <kactioncollection.h>
#include <kstdaction.h>
#include <kicon.h>

#include <KoView.h>
#include <KoZoomAction.h>

#include <KoViewConverter.h>
#include "kis_view2.h"

KisZoomManager::KisZoomManager( KisView2 * view, KoViewConverter * viewConverter )
    : m_view( view )
    , m_viewConverter( viewConverter )
    , m_zoomAction( 0 )
    , m_zoomIn( 0 )
    , m_zoomOut( 0 )
    , m_actualPixels( 0 )
    , m_actualSize( 0 )
    , m_fitToCanvas( 0 )
{
}

KisZoomManager::~KisZoomManager()
{
}

void KisZoomManager::setup( KActionCollection * actionCollection )
{

    // view actions
    m_zoomAction = new KoZoomAction(0, i18n("Zoom"), KIcon("14_zoom"), 0, actionCollection, "zoom" );
    connect(m_zoomAction, SIGNAL(zoomChanged(KoZoomMode::Mode, int)),
          this, SLOT(slotZoomChanged(KoZoomMode::Mode, int)));

    m_zoomIn = KStdAction::zoomIn(this, SLOT(slotZoomIn()), actionCollection, "zoom_in");
    m_zoomOut = KStdAction::zoomOut(this, SLOT(slotZoomOut()), actionCollection, "zoom_out");

/*
    m_actualPixels = new KAction(i18n("Actual Pixels"), actionCollection, "actual_pixels");
    m_actualPixels->setShortcut(Qt::CTRL+Qt::Key_0);
    connect(m_actualPixels, SIGNAL(triggered()), this, SLOT(slotActualPixels()));

    m_actualSize = KStdAction::actualSize(this, SLOT(slotActualSize()), actionCollection, "actual_size");
    m_actualSize->setEnabled(false);
    m_fitToCanvas = KStdAction::fitToPage(this, SLOT(slotFitToCanvas()), actionCollection, "fit_to_canvas");
*/
}


void KisZoomManager::slotZoomChanged(KoZoomMode::Mode mode, int zoom)
{
    KoZoomHandler *zoomHandler = (KoZoomHandler*)m_viewConverter;

    if(mode == KoZoomMode::ZOOM_CONSTANT)
    {
        double zoomF = zoom / 100.0;
        if(zoomF == 0.0) return;
        m_view->setZoom(zoomF);
        zoomHandler->setZoom(zoomF);
    }
    kDebug() << "zoom changed to: " << zoom <<  endl;

    zoomHandler->setZoomMode(mode);
    m_view->canvas()->update();
}

#include "kis_zoom_manager.moc"
