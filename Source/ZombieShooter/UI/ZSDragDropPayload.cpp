// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSDragDropPayload.h"

UZSDragDropPayload* UZSDragDropPayload::Make(FGuid NewInstanceId, EZSDragSourceKind NewSourceKind)
{
	UZSDragDropPayload* Payload = NewObject<UZSDragDropPayload>();
	Payload->InstanceId = NewInstanceId;
	Payload->SourceKind = NewSourceKind;
	return Payload;
}
