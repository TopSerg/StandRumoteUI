#pragma once
/*
* Copyright 2016 Yokogawa Meters & Instruments Corporation.
* FreeRun API Public Header for DL850 Series
*/
#ifdef __cplusplus
extern "C" {
#endif

typedef long ScHandle;

#define SC_SUCCESS 0
#define SC_ERROR   1

// wire type
#define SC_WIRE_USB  7  //!< USBTMC
#define SC_WIRE_LAN  8  //!< EtherNet


/*!
 * \brief envent listener class for receive event
 */
class ScEventListener {
public:
	/*!
	 * \brief DataReady handler
	 * \param handle API handle
	 * \param dataCount count of before LATCH posision
	 */
	virtual void handleEventScDataReady(ScHandle handle, __int64 dataCount){}
};

/*!
 * \brief for DataReady callback
 */
typedef void(__stdcall *ScCallback)(ScHandle handle, int type);

/*!
* \brief begin the use of the API
* \return result
* \retval SC_SUCCESS success
* \retval SC_ERROR error (already initialized)
*/
int ScInit(void);

/*!
* \brief end the use of the API
* \return result
* \retval SC_SUCCESS success
* \retval SC_ERROR error (already end or not initialized)
*/
int ScExit(void);

/*!
* \brief open instrument and get API handle
* \note connect and free run mode
* \param wire wire type
*             (SC_WIRE_USB / SC_WIRE_ETHERNET)
* \param option  serial number / address
*               "192.168.0.1"
* \param rHndl return handle
* \return result
* \retval SC_SUCCESS success
* \retval SC_ERROR error
*/
int ScOpenInstrument(int wire, char* address, ScHandle* rHndl);

/*!
* \brief close instrument (disconnect instrument and move trigger mode)
* \param hndl API handle
* \return result
* \retval SC_SUCCESS success
* \retval SC_ERROR error
*/
int ScCloseInstrument(ScHandle hndl);

/*!
* \brief send command to instrument
* \param hndl API handle
* \param command command
* \return result
* \retval SC_SUCCESS success
* \retval SC_ERROR error
*/
int ScSetControl(ScHandle hndl, char* command);

/*!
* \brief receive command reply from instrument
* \param hndl API handle
* \param buff pointer to receive buffer
* \param buffLen size of receive buffer(byte)
* \param receiveLen receive length
* \return result
* \retval SC_SUCCESS success
* \retval SC_ERROR error
*/
int ScGetControl(ScHandle hndll, char* buff, int buffLen, int* receiveLen);

/*!
* \brief receive binary data from instrument
* \param handle API handle
* \param buff pointer to receive buffer
* \param buffLength size of receive buffer(byte)
* \parma receiveLength receive length
* \retval result
* \retval SC_SUCCESS success
* \retval SC_ERROR error
*/
int ScGetBinaryData(ScHandle handle, char* message, char* buff, int buffLength, int* receiveLength);

/*!
* \brief acquisition start
* \param hndl API handle
* \return result
* \retval SC_SUCCESS success
* \retval SC_ERROR error
*/
int ScStart(ScHandle hndl);

/*!
* \brief acquisition stop
* \param hndl API handle
* \return result
* \retval SC_SUCCESS success
* \retval SC_ERROR error
*/
int ScStop(ScHandle hndl);

/*!
* \brief LATCH on free run
* \param hndl API handle
* \return result
* \retval SC_SUCCESS success
* \retval SC_ERROR error
*/
int ScLatchData(ScHandle hndl);

/*!
* \brief get sample count from LATCH position
* \param handle API handle
* \param count count of sample
* \return result
* \retval SC_SUCCESS success
* \retval SC_ERROR error
*/
int ScGetLatchCount(ScHandle handle, __int64* count);

/*!
* \brief get sample count from previous LATCH position to current LATCH position
* \param handle API handle
* \param count count of sample
* \return result
* \retval SC_SUCCESS success
* \retval SC_ERROR error
*/
int ScGetLatchIntervalCount(ScHandle handle, __int64* count);

/*!
 * \brief get wave data after LATCH
 * \param hndl API handle
 * \param chNo channel number
 * \param subChNo sub channel number
 * \param buff pointer to receive buffer
 * \param bufLen size of receive buffer(byte)
 * \param dataCount count of receive data
 * \param dataSize bit count of one data
 * \return result
 * \retval SC_SUCCESS success
 * \retval SC_ERROR error
 */
int ScGetLatchAcqData(ScHandle hndl, int chNo, int subChNo, char* buff, int bufLen, int* dataCount, int* dataSize);

/*!
* \brief get phase difference of the channel
* \param handle API handle
* \param chNo channel number
* \param delay phase difference
* \return result
* \retval SC_SUCCESS success
* \retval SC_ERROR error
*/
int ScGetChannelDelay(ScHandle handle, int chNo, int* delay);

/*!
* \brief get start time and date
* \param handle API handle
* \param buff pointer to buffer
* \return result
* \retval SC_SUCCESS success
* \retval SC_ERROR error
*/
int ScGetStartTime(ScHandle handle, char* buff);

/*
* \brief set sampling rate
* \param handle API handle
* \param srate sampling rate
* \return result
* \retval SC_SUCCESS success
* \retval SC_ERROR error
*/
int ScSetSamplingRate(ScHandle handle, double srate);

/*
* \brief get sampling rate
* \param handle API handle
* \param srate sampling rate
* \return result
* \retval SC_SUCCESS success
* \retval SC_ERROR error
*/
int ScGetSamplingRate(ScHandle handle, double* srate);

/*!
* \brief get base sampling rate
* \param handle API handle
* \parma srate pointer to valiable of sampling rate
* \return result
* \retval SC_SUCCESS success
* \retval SC_ERROR error
*/
int ScGetBaseSamplingRate(ScHandle handle, double* srate);

/*!
* \brief get sampling ratio from base sampling rate to channel sampling rate
* \param handle API handle
* \parma channel channel number
* \parma ratio sampling ration
* \return result
* \retval SC_SUCCESS success
* \retval SC_ERROR error
*/
int ScGetChannelSamplingRatio(ScHandle handle, int channel, int* ratio);

/*!
 * \brief get data bit count
 * \param chNo channel number(1`16)
 * \param subChNo sub channel number(0:no sub channel, 1-60)
 * \param bits pointer to valiable of bit count
 * \return result
 * \retval SC_SUCCESS success
 * \retval SC_ERROR error
 */
int ScGetChannelBits(ScHandle handle, int chNo, int subChNo, int* bits);

/*!
* \brief get gain value when using convert to scaled value
* \note including liniar scale value
* \param handle API handle
* \parma chNo channel number
* \param subChNo sub channel number(0:no sub channdl, 1-60)
* \parma gain pointer to valiable of gain value
* \return result
* \retval SC_SUCCESS success
* \retval SC_ERROR error
*/
int ScGetChannelGain(ScHandle handle, int chNo, int subChNo, double* gain);

/*!
* \brief get offset value when using convert to scale value
* \note including liniar scale value
* \param handle API handle
* \parma chNo channel number
* \param subChNo sub channel number(0:no sub channel, 1-60)
* \parma offset pointer to valiable of offset value
* \return result
* \retval SC_SUCCESS success
* \retval SC_ERROR error
*/
int ScGetChannelOffset(ScHandle handle, int chNo, int subChNo, double* offset);

/*!
* \brief set data count of DataReady event
* \param handle API handle
* \param sampleCount number of sampling data (1`2^31) (-1:notice of event off)
* \return result
* \retval SC_SUCCESS success
* \retval SC_ERROR error
*/
int ScSetDataReadyCount(ScHandle handle, int sampleCount);

/*!
* \brief get data count of DataReady event
* \param handle API handle
* \param sampleCount pointer to valiable of number of sampling data(1`2^31)(-1:notice of event off)
* \return result
* \retval SC_SUCCESS success
* \retval SC_ERROR error
*/
int ScGetDataReadyCount(ScHandle handle, int* sampleCount);

/*!
* \brief send command to instrument and receive data
* \param handle API handle
* \param message command
* \param buff pointer to receive buffer
* \param buffLength length of buffer
* \param receiveLength receive length
* \return result
* \retval SC_SUCCESS success
* \retval SC_ERROR error
*/
int ScQueryMessage(ScHandle handle, char* message, char* buff, int buffLength, int* receiveLength);

/*!
* \brief add event listener
* \param handle API handle
* \param listener point to event listener object
* \return result
* \retval SC_SUCCESS success
* \retval SC_ERROR error
*/
int ScAddEventListener(ScHandle handle, ScEventListener* listener);

/*!
* \brief delete event listener
* \param handle API handle
* \param listener pointer to event listener object
* \return result
* \retval SC_SUCCESS success
* \retval SC_ERROR error
*/
int ScRemoveEventListener(ScHandle handle, ScEventListener* listener);

#ifdef __cplusplus
}
#endif
