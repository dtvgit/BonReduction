// dllmain.cpp : DLL �A�v���P�[�V�����̃G���g�� �|�C���g���`���܂��B
#include "stdafx.h"
#include "BonTuner.h"

BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch(ul_reason_for_call){
		case DLL_PROCESS_ATTACH:

#ifdef _DEBUG
			// ���������[�N���o�L��
			::_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG));
#endif

			// ���W���[���n���h���ۑ�
			CBonTuner::Module = hModule;
			break;

		case DLL_PROCESS_DETACH:
			// ���J���̏ꍇ�̓C���X�^���X�J��
			if(CBonTuner::This)
			  CBonTuner::This->Release();
			break;
		}

    return TRUE;
}
