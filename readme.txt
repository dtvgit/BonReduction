BonDriver_RDCT ( BonDriver_Reduction )

������

  EDCB �� WOL�@�\�t�� BonDriver_Spinel ��ǂݍ��܂���ƕЂ��[����}�W�b�N�p�P
  �b�g�𑗂�܂��錻�ۂ̉���̂��߂� EDCB �� BonDriver_Spinel �̒��Ԃɑg�ݍ���
  ���߂̃��b�p�[�B

  Spinel �𐔎�q���ɂ��邱�Ƃɂ��A�}�W�b�N�p�P�b�g�̑��M��������邱�Ƃ͉�
  �\�ł͂�����̂́A�}�W�b�N�p�P�b�g�ŋN�������R���s���[�^���X�^���o�C��Ԃ�
  ����� Spinel �����\�P�b�g���ؒf���ꂽ��Ԃ�F�m�ł����Ƀ}�W�b�N�p�P�b�g��
  ����^�C�~���O�������̂ŁA��p�ɂȂ郉�b�p�[�̂悤�Ȃ��̂�����Ă݂��B

  ���̃��b�p�[�𗘗p���邱�Ƃɂ��ABonDriver�̋@�\���J�v�Z�������ďȗ��\��
  ���O�����������󂯂邱�Ƃɂ����BonDriver�C���X�^���X�̐������Ɍ��܂ŗ}����
  ���Ƃ��ł��锤�c

  Spinel���ł̃e�X�g�ł�������m�F���Ă��Ȃ����߉ߓx�Ȋ��҂͋֕��B


���g�p���@

  EDCB �ł̓�����ł̐���

    ���f�B���N�g���\��

    EpgDataCap_Bon.exe
    ��<BonDriver>
       ��BonDriver_RDCT.dll
       ��BonDriver_RDCT.ini
       ��<RDCT>
         ��BonDriver_Spinel.dll.ini
         ��BonDriver_Spinel.dll

    ��BonDriver_RDCT.ini �̓��e

     ----------------------------
     [Setting]
     ;TunerName (�K�{)
     ; �`���[�i�[�̖��O
     TunerName=RDCT:Spinel_PT-T0 �Ƃ�

     ;TunerPath (�K�{)
     ; BonDriver �{�̂̃p�X (���΃p�X�A�܂��̓t���p�X)
     TunerPath=RDCT\BonDriver_Spinel.dll
     ----------------------------

  ��xEDCB�v���O�������N������ƁA.transit.txt �t�@�C���������Ő�������A
  �`�����l����񂪋L�q�����i�ҏW���ă`�����l���̖��O��ς��鎖���\�j�B

  ��x�ڂ̋N������́A.transit.txt �t�@�C���ɏo�͂��ꂽ��񂩂�`���[�i�[��͕�
  ����̂�BonDriver�{�̂̃C���X�^���X�̐������Ɍ��܂ŗ}���邱�Ƃ��ł���悤��
  �Ȃ�B���ӓ_�Ƃ���BonDriver�{�̂̃`�����l������ύX�����ꍇ�� .transit.txt
  �t�@�C�����̂𖕏����Ȃ��ƃ`�����l����񂪂��������Ȃ�̂ŋC�����邱�ƁB

  BonDriver_RDCT.dll �𖼑O��ύX���邱�Ƃɂ���� BonDriver_Spinel �̂悤�ɕ���
  ���\�B���O��ς����ꍇ�� .ini �t�@�C���̖��O���������̂ɂ��邱�ƁB

  ���܂��̂悤�ȋ@�\�Ƃ��ĕ����̃`���[�i�[����̃`���[�i�[�Ƃ��ė��p����
  ���Ƃ��\�B�n�f�W�`���[�i�[��BS/CS�`���[�i�[���~�b�N�X���ĂR�g�Ƃ��Ĉ�������
  ���邱�Ƃ��ł���B

  TVTest �ŎO�g���̗�

    ���f�B���N�g���\��

    TVTest.exe
    ��<BonDriver>
       ��BonDriver_RDCT_PT-TS.dll  �� BonDriver_RDCT.dll �����l�[����������
       ��BonDriver_RDCT_PT-TS.ini  �� BonDriver_RDCT.ini �����l�[����������
       ��<RDCT>
         ��BonDriver_Spinel_PT-T0.dll.ini
         ��BonDriver_Spinel_PT-T0.dll     �� <��> PT2 �� T0 �X�g���[��
         ��BonDriver_Spinel_PT-S0.dll.ini
         ��BonDriver_Spinel_PT-S0.dll     �� <��> PT2 �� S0 �X�g���[��


    ��BonDriver_RDCT.ini �̓��e

     ----------------------------
     [Setting]
     ;TunerName (�K�{)
     ; �`���[�i�[�̖��O
     TunerName=RDCT:Spinel_PT-TS

     ;TunerPath[0..n] ���ł��ǉ��\ (�K�{)
     ; BonDriver �{�̂̃p�X (���΃p�X�A�܂��̓t���p�X)
     TunerPath0=RDCT\BonDriver_Spinel_PT-T0.dll
     TunerPath1=RDCT\BonDriver_Spinel_PT-S0.dll

     ;FullOpen
     ; ��x�J�����`���[�i�[���J�����ςȂ��ɂ��邩�ǂ���
     ; 1 �ɂ���ƃ`���[�i�[�Ԃ̃`�����l���؂�ւ��������ɂȂ邪�A
     ; �J���Ă���`���[�i�[�̕��������\�[�X���Q����
     ; (Default:0)
     FullOpen=1

     ;RecordTransit
     ; .transit.txt �t�@�C���ɑJ�ڂ��L�^���邩�ǂ���
     ; (Default:1)
     RecordTransit=0
     ----------------------------

     �� Version 1.1 ��� TunerPath �� ";" �ŋ�؂邱�Ƃɂ�蕡���̃X�y�A�ƂȂ�
       BonDriver ���ǋL�\�ƂȂ��Ă���B

       �X�y�A�ɂȂ� BonDriver �́A�ŏ��Ɏw�肵�� BonDriver ���J���Ȃ����`���[�j
       ���O�ł��Ȃ��Ƃ��Ɏ����I�ɏz�Q�Ƃ����B

       TunerPath=RDCT/BonDriver_1.dll;RDCT/BonDriver_2.dll;RDCT/BonDriver_3.dll

       ���Ƃ��΁A��L�̏ꍇ�́ABonDriver_1 ���J���Ȃ��Ƃ��́ABonDriver_2 ��
       BonDriver_2 ���J���Ȃ��Ƃ��́ABonDriver_3 ���J���悤�ɏ��X�Ɏ��s����B

       ; �ŋ�؂�ꂽ BonDriver �̌݊����́A�X�y�[�X�ƃ`�����l�����S�������z�u
       ����΁A���S�Ȃ�N���[���łȂ���΂Ȃ�Ȃ����Ƃɒ��ӂ��邱�ƁB


��FAQ

  Q: Spinel ���r���Ŏg�p���A�r���łȂ� BonDriver_Spinel �� BonDriver_RDCT ��
     ���b�v���Ċ����܂��Ďg�p�����TVTest�ɉf�����\������Ȃ��͎̂d�l�Ȃ̂��H
     �r���̃`���[�i�[�Ɠ����`�����l���Ɏ����Ń`���[�j���O�ł��Ȃ��̂��H

  A: �����TVTest�ŗL�̎d�l�Ȃ̂��A�t�@�C������ς��邱�Ƃŉ���ł���悤�ŁA
     ���Ƃ��� BonDriver_RDCT_PT-TS.dll �Ƃ������b�v�����t�@�C�����𗘗p����
     ����ꍇ BonDriver_RDCT_Spinel_PT-TS.dll �ɂ���(���������� Spinel �Ƃ���
     ���O���t�@�C�����Ɋ܂߂�)�݂�Ɖ������邩������Ȃ��B


���X�V����

 version 1.5 rev.5

  �E�����݃^�C�}�[�̐��x�����߂鍀�� MMTimerEnabled ��ini�ɒǉ�[rev.5]
  �E�`���[�i�[�I�[�v���Ď��s���Ƀ`���[�i�[�����񂵖Y��Ă����o�O���C��[rev.4]
  �E�`�����l���ؑ֎��Ƀ`���[�i�[����]�v�ɉ񂵂Ă��܂��o�O���C��[rev.3]
  �E�`���[�i�[���̐ؑւ���肭�����Ȃ��Ȃ邱�Ƃ����錻�ۂ��C��[rev.2]
  �E�`���[�i�[�I�[�v���ɔ�₷�ő厎�s���Ԃ̍��� TunerRetryDuration ��ini�ɒǉ�
  �E�`���[�i�[��Ԃ̕��בւ����o���鍀�� SpaceArrangement ��ini�ɒǉ�
  �E�`���[�i�[���W���[���̈�C�ǂ݂�ݒ肷�鍀�� FullLoad ��ini�ɒǉ�

 version 1.4 rev.4

  �E�񓯊�TS�X�g���[���X���b�h�̗D�揇�ʂ�ݒ�ł��鍀�ڂ�ini�ɒǉ�[rev.4]
  �EdB���ɓ]����Mbps���֕\������ SignalAsBitrate �t���O��ini�ɒǉ�[rev.3]
  �ETransit�t�@�C���� TunerX.SpaceX.ChannelX �� HasSignal �t���O��ǉ�[rev.3]
    ( TunerX.SpaceX.ChannelX.HasSignal=0 �ɂ���Ə���̃V�O�i���𖳌��� )
  �ETransit�t�@�C���� TunerX.SpaceX.ChannelX �� HasStream �t���O��ǉ�[rev.3]
    ( TunerX.SpaceX.ChannelX.HasStream=0 �ɂ���Ə���̃X�g���[�����~ )
  �E�񓯊��o�b�t�@�����O���L���ȏꍇ�ɃA�N�Z�X�ᔽ����������o�O���C��[rev.3]
  �ELazyOpen���L���ȂƂ���TunerPath�ŕ����w�肵�����ׂẴ`���[�i�[���J���Ȃ���
    ���ꍇ�Ƀ`���[�i�[�p�X�𖳌��ɏz�Q�Ƃ��邱�Ƃ̂���o�O���C��[rev.3]
  �E�}�W�b�N�p�P�b�g���M��L��������t���O MagicEnabled ��ini�ɒǉ�[rev.2]
  �E���̃N���C�A���g���`�����l����؂�ւ����Ƃ��Ɏ����Ń`�����l�����`���[�j
    ���O������ ChannelKeeping �t���O��ini�ɒǉ�[rev.1]
  �E�����̃X�y�A�ƂȂ�BonDriver���g���܂킵�Ă���Ƃ��� LazyOpen ���L���ɂȂ�
    �Ă��Ȃ��ꍇ�A�X�y�A��BonDriver��S�����s���Ȃ����Ƃ�����o�O���C��[rev.1]
  �ETS�X�g���[���̔񓯊��o�b�t�����O�@�\��ǉ�

 version 1.3 rev.2

  �E�����̃`���[�i�[���Ǘ����邽�߂� ManageTunerMutex �t���O��ini�ɒǉ�[rev.2]
  �E�����̃`���[�i�[��Ԃ���ɘA�����邽�߂� SpaceConcat �t���O��ini�ɒǉ�

 version 1.2 rev.2

  �ECBonTuner::VirtualToRealSpace ���\�b�h�o�O�C��[rev.2]
  �ECTransitProfiler::Flush ���\�b�h�o�O�C��[rev.1]
  �E.ini �t�@�C���� Transit �Z�N�V�����ɏ����o���Ă����J�ڏ��� .transit.txt
    �t�@�C���ɏ����o���Ǝ����[�`���ɕύX

 version 1.1 rev.2

  �E�`���[�i�[��������_�őJ�ڏ��� Transit �Z�N�V�����Ɉ�Ղɏ����o����
    �����������`���[�i�[�X�y�[�X���̃`�����l���̗񋓂��I��������_�ł��܂߂�
    �����o���悤��ini�t�@�C���ɕۑ�����V���ȏ����̃^�C�~���O��ǉ�[rev.2]
  �E�`�����l���X�L�������s���S�ȏ�Ԃŕۑ����ꂽini�t�@�C��������N������
    �ǂݍ��ނƃv���O�������n���O�A�b�v���N�������Ƃ�����o�O���C��[rev.1]
  �ETunerPath �ɃX�y�A�ƂȂ� BonDriver ��ǋL�ł���悤�ɉ���
  �Eini�t�@�C���ɕۑ�����^�C�~���O���C��

 version 1.0

  �ETransit �Z�N�V���� TunerX.SpaceX �m�[�h�� Visible �t���O��ǉ�
    ( TunerX.SpaceX.Visible=0 �ɂ���Ə���̃X�y�[�X�𖳌��� )
  �ETransit �Z�N�V�����̖��ʏ���������h�~
  �E�`�����l���̐ݒ�Ɏ��s�����ꍇ�A�`���[�i�[�{�̂̃`�����l�����𔲂������
    �����Ă���d�l�ɕύX
  �E�o�[�W�����t���񃊃��[�X


���Ɛӎ���

  ���ۏ� ( NO WARRANTY )


�ȏ�m�V
