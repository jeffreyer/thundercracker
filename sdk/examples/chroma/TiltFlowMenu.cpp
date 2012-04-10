  //---------------------------------------------------------------------------
  // TILT FLOW MENU
  //---------------------------------------------------------------------------

#include "TiltFlowMenu.h"
#include "MenuController.h"
#include "assets.gen.h"
#include "config.h"

const float TiltFlowMenu::UPDATE_DELAY = 1.0f / 30.0f;
const float TiltFlowMenu::REST_DELAY = 0.1f;

TiltFlowMenu *TiltFlowMenu::s_pInst = NULL;


TiltFlowMenu *TiltFlowMenu::Inst()
{
    return s_pInst;
}

TiltFlowMenu::TiltFlowMenu(TiltFlowItem *pItems, int numItems, int numCubes) :
    mNumCubes( numCubes ), mNumItems( numItems )
{
    s_pInst = this;

    Reset();

    mItems = pItems;
}


void TiltFlowMenu::Reset()
{
    mStatus = CHOOSING;
    mSimTime = 0.0f;
    mUpdateTime = 0.0f;
    mPickTime = 0.0f;
    mDone = false;
    mNeighborDirty = true;

    mKeyView = &mViews[0];
    mKeyView->SetStatus(TiltFlowView::STATUS_MENU);
    mKeyView->SetItem(0);
}


void TiltFlowMenu::AssignViews()
{
    //TODO ASSIGN VIEWS
    for( int i = 0; i < mNumCubes; i++ )
	{
        mViews[i].SetCube( &MenuController::Inst().GetWrapper(i)->GetCube() );
	}
}


bool TiltFlowMenu::Tick(float dt)
{
    mSimTime += dt;

    if (mDone) {
        for( int i = 0; i < mNumCubes; i++ )
        {
            mViews[i].Cleanup();
        }
      return false;
    }

    if (mSimTime - mUpdateTime > UPDATE_DELAY) {



      if (mNeighborDirty) {
        mNeighborDirty = false;
        //CheckMenuNeighbors();
      }

      for( int i = 0; i < mNumCubes; i++ )
        mViews[i].Tick();

      if (mStatus == PICKED) {
        mDone = true;
      }

      mUpdateTime = mSimTime;
    }

    for( int i = 0; i < mNumCubes; i++ )
      mViews[i].CheckForRepaint();

    bool bFlush = false;

    //sync and flush
    for( int i = 0; i < mNumCubes; i++ )
    {
        if( mViews[i].IsFlushNeeded() )
        {
            //System::finish();

            //total weirdness.. for some reason if I put a paintSync before the flush as intended,
            //the flush doesn't draw.  Not sure why.
            //System::paintSync();
            mViews[i].Flush();
            System::paintSync();
            //printf( "finishing\n" );
            bFlush = true;
            break;
        }
    }

    /*for( int i = 0; i < mNumCubes; i++ )
    {
        if( mViews[i].IsFlushNeeded() )
        {
            mViews[i].Flush();
        }
    }

    if( bFlush )
        System::paintSync();*/

	return true;
}


void TiltFlowMenu::Pick(TiltFlowView &view)
{
    if (mStatus == CHOOSING) {
      TiltFlowMenu::Inst()->playSound( ui_select );
      //LOG(("Selected {0}", view.Item.name));
      mStatus = PICKED;
      mPickTime = mSimTime;
      for( int i = 0; i < mNumCubes; i++ )
      {
          TiltFlowView &v = mViews[i];
        if (&v == &view) {
          //v.SetStatus(TiltFlowView::STATUS_INFO);
			v.SetStatus(TiltFlowView::STATUS_PICKED);
        } else {
          v.SetStatus(TiltFlowView::STATUS_NONE);
        }
        v.SetDirty();
      }
    }
}


TiltFlowItem *TiltFlowMenu::GetItem( int item )
{
    ASSERT( item >= 0 && item < mNumItems );

    if( item >= 0 && item < mNumItems )
        return &mItems[ item ];

    return NULL;
}

	

//TODO, THIS IS NEIGHBORING, NOT HAPPENING YET
/*
void TiltFlowMenu::CheckMenuNeighbors() {	
	ReassignMenu();
  for( int i = 0; i < mNumCubes; i++ )
  {
      TiltFlowView &view = mViews[i];
    if (&view == mKeyView) {
      view.SetStatus(TiltFlowView::STATUS_MENU);
      view.SetDirty();
    } else {
      view.RelateToMenu();
    }
  }
}



void TiltFlowMenu::ReassignMenu() {
    //if the keyview is alone
    if( HasNeighbors( mKeyView->Cube ) )
        return;

    //then walk through all the cubes, looking for a new key view
    for( int i = 0; i < mNumCubes; i++ )
    {
        TiltFlowView &view = mViews[i];

        if( view.GetStatus() == TiltFlowView::STATUS_ITEM )
            continue;

        if( view.ItemIndex < 0 || view.ItemIndex >= mNumItems )
            continue;

        if (mKeyView.mLastNeighborRemoveSide == Side.LEFT)
        {
            if( view.Cube.Neighbors.Right != NULL || view.Cube.Neighbors.Left == NULL )
                continue;
        }
        else if (mKeyView.mLastNeighborRemoveSide == Side.RIGHT)
        {
            if( view.Cube.Neighbors.Left != NULL || view.Cube.Neighbors.Right == NULL )
                continue;
        }

        //just take the first one
        mKeyView = &view;
    }
}*/


void TiltFlowMenu::playSound( const AssetAudio &sound )
{
#if SFX_ON
    m_SFXChannel.stop();
    m_SFXChannel.play(sound, LoopOnce);
#endif
}


  //---------------------------------------------------------------------------
  // TILT FLOW ITEM
  //---------------------------------------------------------------------------

/*
TiltFlowItem::TiltFlowItem(const Sifteo::AssetImage &image, const char *pName) :
    mName( pName ), mImage( image )
{
}*/

TiltFlowItem::TiltFlowItem(const Sifteo::AssetImage &image, const Sifteo::AssetImage &label) :
    mImage( image ), mLabel( label )
{
}

  //---------------------------------------------------------------------------
  // TILT FLOW VIEW
  //---------------------------------------------------------------------------

const float TiltFlowView::DRIFTVEL = 2.0f;
const float TiltFlowView::TILTVEL = 6.0f;
//const float TiltFlowView::TILTVEL = 0.6f;
const float TiltFlowView::MINACCEL = 3.0f;
const float TiltFlowView::MAXACCEL = 8.0f;
const float TiltFlowView::DEEPTILTACCEL = 2.0f;
const float TiltFlowView::GRAVITY = 1.03f;
const float TiltFlowView::EPSILON = 0.00001f;
const float TiltFlowView::TIP_DELAY = 2.0f;

const Sifteo::AssetImage *TiltFlowView::TIPS[NUM_TIPS] =
{
    &UI_Main_Menu_TipsTilt,
    &UI_Main_Menu_TipsTouch,
};

int TiltFlowView::s_cubeIndex = 0;


TiltFlowView::TiltFlowView() :
    mItem( -1 ), mOffsetX( 0.0f ), mAccel( MINACCEL ),
    mRestTime( -1.0f ), mDrawLabel( true ), mDirty( true ),
    mCurrentTip( 0 ), mTipTime(0.0f), mLastNeighborRemoveSide( NONE ), mpCube( NULL ),
    mLastNeighboredSide( NONE ), mFlushNeeded( false )
{
}


void TiltFlowView::CheckForRepaint() {
  if (mDirty) {
    Paint();
    mDirty = false;
  }
}

void TiltFlowView::Tick() {
  bool wasDrawingLabel = mDrawLabel;
  switch(mStatus) {
  case STATUS_MENU:
    UpdateMenu();
    break;
  case STATUS_ITEM:
    CoastToStop();
    break;
  default:
    // do nothing
    break;
  }

  if (mRestTime > -1 && TiltFlowMenu::Inst()->GetSimTime() - mRestTime > TiltFlowMenu::REST_DELAY) {
    mDirty = true;
    mDrawLabel = true;

    if (mDrawLabel && !wasDrawingLabel) {
      TiltFlowMenu::Inst()->playSound( settle );
    }
    mRestTime = -1;
  }

  if (TiltFlowMenu::Inst()->GetSimTime() - mTipTime > TIP_DELAY)
  {
      mCurrentTip = ( mCurrentTip + 1 ) % NUM_TIPS;
      mTipTime = TiltFlowMenu::Inst()->GetSimTime();
      mDirty = true;
  }
}


_SYSCubeID TiltFlowView::getNeighbor()
{
	for( int i = 0; i < NUM_SIDES; i++ )
	{
		if( mpCube->hasPhysicalNeighborAt(i) )
		{
			mLastNeighboredSide = (Side)i;
			return mpCube->physicalNeighborAt(i);
		}
	}

	return CUBE_ID_UNDEFINED;
}


void TiltFlowView::Paint()
{
    switch(mStatus) {
    case STATUS_MENU:
      PaintMenu();
      break;
    case STATUS_ITEM:
      PaintItem();
      break;
    case STATUS_INFO:
      PaintInfo();
      break;
    default:
      PaintNone();
      break;
    }
}


void TiltFlowView::PaintNone() {
  VidMode_BG0 vid( mpCube->vbuf );
  vid.clear(White.tiles[0]);
}

void TiltFlowView::PaintInfo() {
   /*
  if (TiltFlowMenu::Inst()->PaintBackground != NULL) { TiltFlowMenu::Inst()->PaintBackground(this); } else { c.FillScreen(Color.White); }
  DoPaintItem(Item, 24, 80); // magic
  TiltFlowMenu::Inst()->Font.Paint(c, Item.name, vec(0,0), HorizontalAlignment.Center, VerticalAlignment.Middle, 1, 0, true, false, new vec(128,24)); // magic
  */
}

const float SCROLL_AMOUNT = 220.0f;

Float2 SCROLL_DIRS[] = {
	{ 0.0f, SCROLL_AMOUNT },
	{ SCROLL_AMOUNT, 0.0f },
	{ 0.0f, -SCROLL_AMOUNT },
	{ -SCROLL_AMOUNT, 0.0f },
};

void TiltFlowView::PaintMenu() {
  //if (TiltFlowMenu::Inst()->PaintBackground != NULL) { TiltFlowMenu::Inst()->PaintBackground(this); } else { c.FillScreen(Color.White); }

  //could be better, but just clear for now
  VidMode_BG0 vid( mpCube->vbuf );
  vid.clear(White.tiles[0]);

  //DEBUG DRAW
  /*for( unsigned int i = 0; i < VidMode_BG0::BG0_width; i++ )
  {
      vid.BG0_textf( vec( i, 0 ), Font, "%d", i%10 );
  }*/

  Int2 offset = { 24, 0 };

  DoPaintItem(TiltFlowMenu::Inst()->GetItem( mItem ), offset.x, offset.y);

  if( mStatus != STATUS_PICKED )
  {
      BG1Helper &bg1helper = MenuController::Inst().GetWrapper( mpCube )->getBG1Helper();

	  if (/*c.Neighbors.Left == NULL && */mItem > 0) {
		DoPaintItem(TiltFlowMenu::Inst()->GetItem( mItem - 1 ), -70);
	  }
	  if (/*c.Neighbors.Right == NULL && */mItem < TiltFlowMenu::Inst()->GetNumItems()-1) {
		DoPaintItem(TiltFlowMenu::Inst()->GetItem( mItem + 1), 118);
	  }

	  if (mDrawLabel) {
		//TiltFlowMenu::Inst()->Font.Paint(c, Item.name, vec(0,0), HorizontalAlignment.Center, VerticalAlignment.Middle, 1, 0, true, false, new vec(128, 20)); // magic
		  bg1helper.DrawAsset(vec(LABEL_OFFSET,0), TiltFlowMenu::Inst()->GetItem( mItem )->mLabel);
	  }

	  //draw the current tip
	  bg1helper.DrawAsset(vec(0,TIP_Y_OFFSET), *TIPS[mCurrentTip]);

      //bg1helper.Flush();
      mFlushNeeded = true;
      //printf ("would have flushed here\n");
  }



  // Firmware handles all pixel-level scrolling
  vid.BG0_setPanning(vec((int)-mOffsetX, COVER_Y_OFFSET - offset.y));
  //TODO, fix hack!
  mpCube->vbuf.touch();
}

void TiltFlowView::DoPaintItem(TiltFlowItem *pItem, int x, int y) {
  if( pItem )
  {
      VidMode_BG0 vid( mpCube->vbuf );
      Int2 offset = vec( 0, 0 );
      Int2 size = vec( pItem->mImage.width, pItem->mImage.height );

      int leftMostTile = -mOffsetX / 8 - 1;
      int rightMostTile = leftMostTile + VidMode_BG0::BG0_width - 1;
      int curTile = x / 8;

      if( curTile > rightMostTile || curTile + (int)pItem->mImage.width < leftMostTile )
          return;

      if( curTile < leftMostTile )
      {
          offset.x = leftMostTile-curTile;
          size.x -= offset.x;
          curTile = leftMostTile;
      }
      else if( curTile + pItem->mImage.width > (unsigned)rightMostTile + 1 )
      {
          size.x -= curTile + pItem->mImage.width - rightMostTile;
      }

      ASSERT( size.x >= 0 );

	  //y axis handled completely differently, since we aren't panning on y
	  //just clip y to 0 and bottom of screen
	  //CES HACKERY
	  int yTile = y / 8;
	  int yOffset = 0;

	  if( yTile + size.y <= -3 )
		  return;
	  else if( yTile < -3 )
	  {
		  int diff = -3 - yTile;
		  offset.y += diff;
		  size.y -= diff;
		  yOffset += diff;
	  }
	  else if( yTile + pItem->mImage.height > VidMode_BG0::BG0_height - 4 )
	  {
		  int diff = ( yTile + pItem->mImage.height ) - ( VidMode_BG0::BG0_height - 4 );

		  if( diff >= size.y )
			  return;
		  size.y -= diff;
	  }

      //this shouldn't ever really loop, since we're heavily constraining curTile
      while( curTile < 0 )
      {
          curTile += VidMode_BG0::BG0_width;
      }

      //need to draw on the right tiles.
      if( curTile + size.x < (int)VidMode_BG0::BG0_width )
        vid.BG0_drawPartialAsset(vec(curTile,yOffset), offset, size, pItem->mImage, 0);
      //Handle wrapping
      else
      {
          int wrapAmt = curTile + size.x - VidMode_BG0::BG0_width;
          size.x -= wrapAmt;
          vid.BG0_drawPartialAsset(vec(curTile,yOffset), offset, size, pItem->mImage, 0);
          offset.x += size.x;
          size.x = wrapAmt;
          vid.BG0_drawPartialAsset(vec(0,yOffset), offset, size, pItem->mImage, 0);
      }
  }
}

void TiltFlowView::PaintItem() {
  /*if (TiltFlowMenu::Inst()->PaintBackground != NULL) { TiltFlowMenu::Inst()->PaintBackground(this); } else { c.FillScreen(Color.White); }
  int x, w;
  ClipIt(24, out x, out w); // magic
  if (w > 0) {
    DoPaintItem(Item, x, w);
    if (mDrawLabel) {
      TiltFlowMenu::Inst()->Font.Paint(c, Item.name, vec(0,0), HorizontalAlignment.Center, VerticalAlignment.Middle, 1, 0, true, false, new vec(128,20)); // magic
    }
  }*/
}


Int2 TiltFlowView::LerpPosition( const Int2 &start, const Int2 &end, float timePercent )
{
	if( timePercent < 0.0f )
		timePercent = 0.0f;
	if( timePercent > 1.0f )
		timePercent = 1.0f;
    Int2 result = { start.x + ( end.x - start.x ) * timePercent, start.y + ( end.y - start.y ) * timePercent };

    return result;
}

void TiltFlowView::OnButton(bool pressed) {
  if (TiltFlowMenu::Inst()->GetStatus() == TiltFlowMenu::CHOOSING && mStatus != STATUS_NONE) {
    StopScrolling();
    TiltFlowMenu::Inst()->Pick(*this);
    mDirty = true;
  }
}


//TODO need flip
/*
void TiltFlowView::OnFlip(Cube c, bool newOrientationIsUp) {
  if (!newOrientationIsUp) {
    TiltFlowMenu::Inst()->CheckFlip();
  }
}*/


//TODO, remove neighboring for now
/*
void TiltFlowView::OnNeighborAdd(Cube c, Side s, Cube nc, Side ns) {
  if (TiltFlowMenu::Inst()->GetStatus() == TiltFlowTiltFlowMenu::Inst()->CHOOSING) {
    TiltFlowMenu::Inst()->DirtyNeighbors();
  }
}

void TiltFlowView::OnNeighborRemove(Cube c, Side s, Cube nc, Side ns) {
  if (TiltFlowMenu::Inst()->GetStatus() == TiltFlowTiltFlowMenu::Inst()->CHOOSING) {
    mLastNeighborRemoveSide = s;
    TiltFlowMenu::Inst()->DirtyNeighbors();
  }
}

void TiltFlowView::RelateToMenu() {
  Status oldStatus = mStatus;
  int oldItem = mItem;
  Int2 pos;
  if (TiltFlowMenu::Inst()->GetKeyView()->PositionOf(Cube, out pos)) {
    if (pos.y == 0) {
      mItem = TiltFlowMenu::Inst()->GetKeyView()->mItem + pos.x;
      if (mItem >= 0 && mItem < TiltFlowMenu::Inst()->GetNumItems()) {
        if (mStatus != STATUS_ITEM) {
          mStatus = STATUS_ITEM;
          mAccel = MAXACCEL;
          mDrawLabel = false;
          if (pos.x > 0) {
            mOffsetX = -40; // magic
          } else {
            mOffsetX = 40; // magic
          }
        }
      } else {
        mStatus = STATUS_NONE;
      }
    } else if (pos.y == -1 || pos.y == 1) {
      mItem = TiltFlowMenu::Inst()->GetKeyView()->mItem + pos.x;
      if (mItem >= 0 && mItem < TiltFlowMenu::Inst()->GetNumItems()) {
        mStatus = STATUS_INFO;
        mOffsetX = 0;
      } else {
        mStatus = STATUS_NONE;
      }
    } else {
      mStatus = STATUS_NONE;
    }
  } else {
    mStatus = STATUS_NONE;
  }

  //TODO SOUND
  if (mItem != oldItem || mStatus != oldStatus) {
    Hacky.Sfx("ui_select_02");
  }
  mDirty |= mStatus != oldStatus;
  mDirty |= mItem != oldItem;
}
  */

void TiltFlowView::UpdateMenu() {
  Cube::TiltState state = mpCube->getTiltState();

  //TEMP DEBUG, use neighboring instead of tilt
  /*if( mpCube->hasPhysicalNeighborAt(RIGHT) )
      state.x = _SYS_TILT_POSITIVE;
  else if( mpCube->hasPhysicalNeighborAt(LEFT) )
      state.x = _SYS_TILT_NEGATIVE;
  else
      state.x = _SYS_TILT_NEUTRAL;
*/

  if (
    state.x == _SYS_TILT_NEUTRAL ||
    (state.x == _SYS_TILT_NEGATIVE && mItem == TiltFlowMenu::Inst()->GetNumItems()-1 && mOffsetX >= 0) ||
    (state.x == _SYS_TILT_POSITIVE && mItem == 0 && mOffsetX <= 0)
  ) {
    CoastToStop();
  } else {
    mRestTime = TiltFlowMenu::Inst()->GetSimTime();

    // accelerate in the direction of tilt
    if (mDrawLabel) {
      TiltFlowMenu::Inst()->playSound( changeoption );
    }
    mDrawLabel = false;
    float vSign = state.x == _SYS_TILT_NEGATIVE ? -1.0f : 1.0f;

    // if we're way overtilted (into the upper corners!), accelerate super-fast
    float deepMult = state.y == _SYS_TILT_POSITIVE ? DEEPTILTACCEL : 1.0f;

    if (vSign > 0) {
      if (mItem > 0) {
        if (mOffsetX > 45) { // magic
          mItem--;
          mOffsetX -= 90; // magic
        } else {
            mOffsetX += Resistance() * TILTVEL * mAccel;
          mAccel = Util::Clamp(mAccel * GRAVITY, MINACCEL, MAXACCEL) * deepMult;
        }
      }
    } else {
      if (mItem < TiltFlowMenu::Inst()->GetNumItems()-1) {
        if (mOffsetX < -45) { // magic
          mItem++;
          mOffsetX += 90; // magic
        } else {
            mOffsetX -= Resistance() * TILTVEL * mAccel;
          mAccel = Util::Clamp(mAccel * GRAVITY, MINACCEL, MAXACCEL) * deepMult;
        }
      }
    }
    mDirty = true;
  }

  //check for touch
  if( mpCube->touching() )
  {
      TiltFlowMenu::Inst()->Pick( *this );
  }
}

float TiltFlowView::Resistance() {
    float u = 1.0f - abs(mOffsetX / 45.0f);
    return 1.0f - 0.95f * (u * u);
}

void TiltFlowView::CoastToStop() {
  if (abs(mOffsetX)  > EPSILON ) {
    mAccel = Util::Clamp(mAccel/GRAVITY, MINACCEL, MAXACCEL);
    mRestTime = TiltFlowMenu::Inst()->GetSimTime();
    if (abs(mOffsetX) < MINACCEL) { // magic
      StopScrolling();
    } else if (mOffsetX > 0.0f) {
          mOffsetX -= DRIFTVEL * mAccel;
          if (mOffsetX < 0) { mAccel = MINACCEL; }
    } else {
          mOffsetX += DRIFTVEL * mAccel;
          if (mOffsetX > 0) { mAccel = MINACCEL; }
    }
    mDirty = true;
  }
}

void TiltFlowView::StopScrolling() {
  mOffsetX = 0;
  mAccel = MINACCEL;
  mRestTime = TiltFlowMenu::Inst()->GetSimTime();
  mDrawLabel = false;
  mDirty = true;
}


/*
  TODO
  neighboring doesn't happen for tiltflow now

bool PositionOf(Cube c, out Int2 pos) {
  if (Cube == c) {
    pos = vec(0,0);
    return true;
  }
  if (Cube == NULL || Cube.Neighbors.IsEmpty) {
    pos = vec(0,0);
    return false;
  }
  foreach(var view in TiltFlowMenu::Inst()->Views) { view.mVisited = false; }
  mGridPosition = vec(0,0);
  mVisited = true;
  for(int i=0; i<4; ++i) {
    Int2 p;
    if (PositionOfVisit(this, c, (Side)i, out p)) {
      pos = p;
      return true;
    }
  }
  pos = vec(0,0);
  return false;
}

bool PositionOfVisit(TiltFlowView origin, Cube target, Side s, out Int2 result) {
  var n = origin.Cube.Neighbors[s];
  if (n == NULL) {
    result = vec(0,0);
    return false;
  };
  var view = n.userData as TiltFlowView;
  if (view.mVisited) {
    result = vec(0,0);
    return false;
  }
  view.mVisited = true;
  view.mGridPosition = origin.mGridPosition + Int2.Side(s);
  if (view.Cube == target) {
    result = view.mGridPosition;
    return true;
  } else {
    for(int i=0; i<4; ++i) {
      Int2 p;
      if (PositionOfVisit(view, target, (Side)i, out p)) {
        result = p;
        return true;
      }
    }
    result = vec(0,0);
    return false;
  }

}

}

*/


void TiltFlowView::Flush()
{
    //printf( "flushing\n" );
    MenuController::Inst().GetWrapper( mpCube )->getBG1Helper().Flush();
    //force touch
    //MenuController::Inst().cubes[ mpCube ].GetCube().vbuf.touch();
    mFlushNeeded = false;
}



void TiltFlowView::Cleanup()
{
    VidMode_BG0 vid( mpCube->vbuf );
    vid.BG0_setPanning(vec(0, 0));
}