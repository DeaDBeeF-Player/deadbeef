#include "CEnvelope.hpp"

#if defined (_MSC_VER)
#if defined (_DEBUG)
#define new new( _CLIENT_BLOCK, __FILE__, __LINE__)
#endif
#endif

using namespace dsa;

#define GETA_BITS 20
#define MAX_CNT (1<<(GETA_BITS+8))

UINT32 CEnvelope::_CalcSpeed(UINT32 ms) {
  if(ms==0)
    return MAX_CNT;
  else
    return (MAX_CNT/(ms*m_clock/1000)) * m_rate;
}

CEnvelope::CEnvelope(UINT ch) : m_ch(ch) {
  m_ci = new ChannelInfo[ch];
}

CEnvelope::~CEnvelope() {
  delete [] m_ci;
}

void CEnvelope::Reset(DWORD clock, DWORD rate) {
  m_rate = rate;
  m_clock = clock;
  m_cnt = 0;
  m_inc = (MAX_CNT/m_clock) * m_rate;
  for(UINT ch=0; ch<m_ch; ch++) {
    m_ci[ch].value = 0;
    m_ci[ch].speed = 0;
    m_ci[ch].state = FINISH;
  }
}

bool CEnvelope::Update() {

  m_cnt += m_inc;
  if(m_cnt < MAX_CNT) return false;
  m_cnt &= 0xFFFFFFF;

  for(UINT ch=0; ch<m_ch; ch++) {

    switch(m_ci[ch].state) {
    case ATTACK:
      if( m_ci[ch].speed + m_ci[ch].value < MAX_CNT )
        m_ci[ch].value += m_ci[ch].speed;
      else {
        m_ci[ch].value = MAX_CNT;
        m_ci[ch].speed = _CalcSpeed(m_ci[ch].param.dr);
        m_ci[ch].state = DECAY;
      }
      break;
    case DECAY:
      if(( m_ci[ch].value > m_ci[ch].speed )&&
         ( m_ci[ch].value > (UINT32)m_ci[ch].param.sl<<GETA_BITS)) {
        m_ci[ch].value -= m_ci[ch].speed;
      }
      else {
        m_ci[ch].speed = _CalcSpeed(m_ci[ch].param.sr);
        m_ci[ch].value = (UINT32)m_ci[ch].param.sl<<GETA_BITS;
        m_ci[ch].state = SUSTINE;
      }
      break;
    case SUSTINE:
      if(( m_ci[ch].speed > m_ci[ch].value )) {
        m_ci[ch].value = 0;
        m_ci[ch].state = FINISH;
      } else {
        m_ci[ch].value -= m_ci[ch].speed;
      }
      break;
    case RELEASE:
      if(( m_ci[ch].speed > m_ci[ch].value )) {
        m_ci[ch].value = 0;
        m_ci[ch].state = FINISH;
      } else {
        m_ci[ch].value -= m_ci[ch].speed;
      }
      break;
    default:
      break;
    }
  }

  return true;
}

void CEnvelope::KeyOn(UINT ch) {
  m_ci[ch].value = 0;
  m_ci[ch].speed = _CalcSpeed(m_ci[ch].param.ar);
  m_ci[ch].state = ATTACK;
}

void CEnvelope::KeyOff(UINT ch) {
  m_ci[ch].state = RELEASE;
  m_ci[ch].speed = _CalcSpeed(m_ci[ch].param.rr);
}

void CEnvelope::SetParam(UINT ch, const Param& param) { 
  m_ci[ch].param = param; 
}

UINT32 CEnvelope::GetValue(UINT ch) const {
  return m_ci[ch].value >> GETA_BITS;
}