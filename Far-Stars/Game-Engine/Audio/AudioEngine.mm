//
//#import "AudioEngine.h"
//
//@implementation AudioPlayer {
//    
//    AVAudioPlayerNode * _playerNode;
//    AVAudioEnvironmentNode * _environmentNode;
//    AVAudioUnitReverb * _reverbUnit;
//    
//}
//
//// Plays an audio file - Pass in ONLY FILE NAME
//// Ex: @"laugh", NOT: @"users/desktop/laugh.wav"
//// Sample code is shown below on how to initiate completionHandler
///// AVAudioNodeCompletionHandler completionHandler = ^{
/////   NSLog(@"Done");
///// };
//// The starting at variable will need to be negative to skip and positive to have a delay
//- (void)initWithEngine:(AVAudioEngine *)engine at:(NSString *)audioURL withCompletion:(AVAudioNodeCompletionHandler)completionHandler startingAt:(float)delay
//{
//    
//    NSError *error = nil;
//    
//    NSURL * URL = [NSURL fileURLWithPath:[[NSBundle mainBundle] pathForResource:audioURL ofType:@"wav"]];
//    NSLog(@"%@",URL);
//    AVAudioFile *audioFile = [[AVAudioFile alloc] initForReading:URL error:&error];
//    if (!audioFile) NSLog(@"Error loading audio file: %@", error.localizedDescription);
//    
//    AVAudioPCMBuffer *buffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:[audioFile processingFormat] frameCapacity:(unsigned int)[audioFile length]];
//    if (!buffer) NSLog(@"Failed to allocate buffer");
//    
//    //        [audioFile readIntoBuffer:buffer error:&error];
//    BOOL success = [audioFile readIntoBuffer:buffer error:&error];
//    if (!success || error) {
//        NSLog(@"Error reading audio file into buffer: %@", error.localizedDescription);
//        return;
//    }
//    
//    _playerNode = [[AVAudioPlayerNode alloc] init];
//    _environmentNode = [[AVAudioEnvironmentNode alloc] init];
//    _reverbUnit = [[AVAudioUnitReverb alloc] init];
//    [engine attachNode:_playerNode];
//    [engine attachNode:_environmentNode];
//    [engine attachNode:_reverbUnit];
//    
//    _reverbUnit.wetDryMix = 0; // 0 - 100% float
//    
//    [engine connect:_playerNode to:_environmentNode format:buffer.format];
//    [engine connect:_environmentNode to:_reverbUnit format:[_environmentNode outputFormatForBus:0]];
//    [engine connect:_reverbUnit to:engine.mainMixerNode format:[_reverbUnit outputFormatForBus:0]];
//    
//    AVAudioTime * playerNodeDelay = nil;
//    
//    if(delay!=0) {
//        AVAudioFramePosition startSampleTime = _playerNode.lastRenderTime.sampleTime + delay * [_playerNode outputFormatForBus:0].sampleRate;
//        playerNodeDelay = [AVAudioTime timeWithSampleTime:startSampleTime atRate:[_playerNode outputFormatForBus:0].sampleRate];
//    }
//    
//    [_playerNode scheduleBuffer:buffer atTime:playerNodeDelay options:AVAudioPlayerNodeBufferInterruptsAtLoop completionHandler:completionHandler];
//    //[_playerNode scheduleBuffer:buffer completionHandler:completionHandler];
//    
//    [engine prepare];
//    //[engine startAndReturnError:nil];
//    NSError *startError = nil;
//    if (![engine startAndReturnError:&startError]) {
//        NSLog(@"Error starting audio engine: %@", startError.localizedDescription);
//        return;
//    }
//    
//}
//
//
//
//// Configure wet dry mix
//- (void)setWetDryMixTo:(float)newWetDryMix // 0 - 100%
//{
//    _reverbUnit.wetDryMix = newWetDryMix;
//}
//
//
//// Configure angle of the listener
//- (void)setListenerAngleTo:(AVAudio3DAngularOrientation)newAngle
//{
//    // -180 - 180
//    // Yaw, pitch, roll
//    _environmentNode.listenerAngularOrientation = newAngle;
//}
//
//
//// Configure position of listener
//- (void)setListenerPositionTo:(AVAudio3DPoint)newPos
//{
//    _environmentNode.listenerPosition = newPos;
//}
//
//// Start the audio after configuration
//- (void)startAudio
//{
//    [_playerNode play];
//}
//
//
//@end
